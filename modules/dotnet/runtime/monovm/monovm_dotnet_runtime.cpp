/**************************************************************************/
/*  monovm_dotnet_runtime.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifdef DOTNET_MONOVM_ENABLED

#include "monovm_dotnet_runtime.h"

#include "../../utils/dirs_utils.h"
#include "monovm_dotnet_config.h"

#include "core/io/file_access.h"
#include "core/os/os.h"

#include "../../thirdparty/mono_delegates.h"

namespace DotNet {

// TODO: Have to use 'coreclr_create_delegate' instead of the mono delegate because
// the mono delegate's symbol is not exported.
// See: https://github.com/dotnet/runtime/pull/107778
coreclr_create_delegate_fn coreclr_create_delegate = nullptr;
monovm_initialize_fn monovm_initialize = nullptr;

mono_install_assembly_preload_hook_fn mono_install_assembly_preload_hook = nullptr;
mono_assembly_name_get_name_fn mono_assembly_name_get_name = nullptr;
mono_assembly_name_get_culture_fn mono_assembly_name_get_culture = nullptr;
mono_image_open_from_data_with_name_fn mono_image_open_from_data_with_name = nullptr;
mono_assembly_load_from_full_fn mono_assembly_load_from_full = nullptr;

bool MonoVMDotNetRuntime::try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const {
	String assembly_path = p_assemblies_directory.path_join(p_assembly_name + ".dll");

	Ref<MonoVMDotNetConfig> config;
	config.instantiate();
	config->assembly_name = p_assembly_name;
	config->assembly_path = assembly_path;
	// TODO: Assume the entry-point is in a 'Main' class under a namespace that matches the assembly name,
	// and the method is named 'Init'. In the future we may want to provide a way to customize the entry-point
	// in project settings.
	config->entry_point_class_name = p_assembly_name + ".Main";
	config->entry_point_method_name = "Init";

	r_extension_config = config;
	return true;
}

bool MonoVMDotNetRuntime::try_initialize() {
	if (monovm_dll_handle != nullptr) {
		// Already initialized.
		return true;
	}

	return try_load_monovm(monovm_dll_handle);
}

bool MonoVMDotNetRuntime::try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	const Ref<MonoVMDotNetConfig> config = p_extension_config;
	if (config.is_null()) {
		return false;
	}

	String assemblies_directory = config->assembly_path.get_base_dir();

	return initialize_monovm(assemblies_directory);
}

GDExtensionInitializationFunction MonoVMDotNetRuntime::get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	ERR_FAIL_NULL_V_MSG(coreclr_create_delegate, nullptr, ".NET: MonoVM is not initialized.");
	const Ref<MonoVMDotNetConfig> config = p_extension_config;
	ERR_FAIL_COND_V_MSG(config.is_null(), nullptr, ".NET: Extension configuration is incompatible with MonoVM.");

	GDExtensionInitializationFunction entry_point = nullptr;

	String assembly_name = config->assembly_name;
	String type_name = config->entry_point_class_name;
	String method_name = config->entry_point_method_name;

	int rc = coreclr_create_delegate(
			nullptr, 0,
			assembly_name.utf8().get_data(),
			type_name.utf8().get_data(),
			method_name.utf8().get_data(),
			(void **)&entry_point);
	ERR_FAIL_COND_V_MSG(rc != 0, nullptr, ".NET: Failed to get entry-point function pointer with code: " + itos(rc));

	return entry_point;
}

void MonoVMDotNetRuntime::close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	// In MonoVM we don't use Godot.PluginLoader to load assemblies
	// so we can't unload extensions. But that's fine because we only
	// use this runtime for exported games, and there shouldn't be any
	// assembly unloading/reloading in an exported game.
}

MonoAssembly *MonoVMDotNetRuntime::load_assembly_from_pck(MonoAssemblyName *p_assembly_name, char **p_assemblies_path, void *p_user_data) {
	constexpr bool ref_only = false;

	const char *name = mono_assembly_name_get_name(p_assembly_name);
	const char *culture = mono_assembly_name_get_culture(p_assembly_name);

	String assembly_name;
	if (culture && strcmp(culture, "")) {
		assembly_name += culture;
		assembly_name += "/";
	}
	assembly_name += name;
	if (!assembly_name.ends_with(".dll")) {
		assembly_name += ".dll";
	}

	String path = Dirs::get_project_assemblies_path();
	path = path.path_join(assembly_name);

	print_verbose(".NET: Loading assembly '" + assembly_name + "' from '" + path + "'.");

	if (!FileAccess::exists(path)) {
		// We could not find the assembly, return null so another hook may find it.
		return nullptr;
	}

	Vector<uint8_t> data = FileAccess::get_file_as_bytes(path);
	ERR_FAIL_COND_V_MSG(data.is_empty(), nullptr, ".NET: Could not read assembly in '" + path + "'.");

	MonoImageOpenStatus status = MONO_IMAGE_OK;

	MonoImage *image = mono_image_open_from_data_with_name(
			reinterpret_cast<char *>(data.ptrw()), data.size(),
			/*need_copy*/ true,
			&status,
			ref_only,
			assembly_name.utf8().get_data());

	ERR_FAIL_COND_V_MSG(status != MONO_IMAGE_OK || image == nullptr, nullptr, ".NET: Failed to open assembly image.");

	status = MONO_IMAGE_OK;

	MonoAssembly *assembly = mono_assembly_load_from_full(
			image, assembly_name.utf8().get_data(),
			&status,
			ref_only);

	ERR_FAIL_COND_V_MSG(status != MONO_IMAGE_OK || assembly == nullptr, nullptr, ".NET: Failed to load assembly from image.");

	return assembly;
}

String MonoVMDotNetRuntime::find_monosgen() {
#if defined(ANDROID_ENABLED)
	// Android includes all native libraries in the libs directory of the APK
	// so we assume it exists and use only the name to dlopen it.
	return "libmonosgen-2.0.so";
#else

#if defined(WINDOWS_ENABLED)
	String monosgen_path = Dirs::get_project_assemblies_path().path_join("monosgen-2.0.dll");
#elif defined(MACOS_ENABLED)
	String monosgen_path = Dirs::get_project_assemblies_path().path_join("libmonosgen-2.0.dylib");
#elif defined(UNIX_ENABLED)
	String monosgen_path = Dirs::get_project_assemblies_path().path_join("libmonosgen-2.0.so");
#else
#error "Platform not supported (yet?)"
#endif

	if (FileAccess::exists(monosgen_path)) {
		return monosgen_path;
	}

	return String();

#endif // ANDROID_ENABLED
}

bool MonoVMDotNetRuntime::try_load_monovm(void *&r_monovm_dll_handle) {
	String monosgen_path = find_monosgen();

	if (monosgen_path.is_empty()) {
		return false;
	}

	print_verbose("Found monosgen: " + monosgen_path);

	Error err = OS::get_singleton()->open_dynamic_library(monosgen_path, r_monovm_dll_handle);

	if (err != OK) {
		return false;
	}

	void *lib = r_monovm_dll_handle;

	void *symbol = nullptr;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "monovm_initialize", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	monovm_initialize = (monovm_initialize_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "coreclr_create_delegate", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	coreclr_create_delegate = (coreclr_create_delegate_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "mono_install_assembly_preload_hook", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	mono_install_assembly_preload_hook = (mono_install_assembly_preload_hook_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "mono_assembly_name_get_name", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	mono_assembly_name_get_name = (mono_assembly_name_get_name_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "mono_assembly_name_get_culture", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	mono_assembly_name_get_culture = (mono_assembly_name_get_culture_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "mono_image_open_from_data_with_name", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	mono_image_open_from_data_with_name = (mono_image_open_from_data_with_name_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "mono_assembly_load_from_full", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	mono_assembly_load_from_full = (mono_assembly_load_from_full_fn)symbol;

	return (monovm_initialize &&
			coreclr_create_delegate);
}

bool MonoVMDotNetRuntime::initialize_monovm(const String &p_assemblies_directory) {
#ifdef ANDROID_ENABLED
	// Android requires installing a preload hook to load assemblies from inside the APK,
	// other platforms can find the assemblies with the default lookup.
	if (mono_install_assembly_preload_hook != nullptr) {
		mono_install_assembly_preload_hook(&MonoVMDotNetRuntime::load_assembly_from_pck, nullptr);
	}
#endif

	int rc = monovm_initialize(0, nullptr, nullptr);
	ERR_FAIL_COND_V_MSG(rc != 0, false, "monovm_initialize failed with code: " + itos(rc));

	return true;
}

MonoVMDotNetRuntime::~MonoVMDotNetRuntime() {
	if (monovm_dll_handle != nullptr) {
		OS::get_singleton()->close_dynamic_library(monovm_dll_handle);
	}
}

} // namespace DotNet

#endif // DOTNET_MONOVM_ENABLED
