/**************************************************************************/
/*  hostfxr_dotnet_runtime.cpp                                            */
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

#ifdef DOTNET_HOSTFXR_ENABLED

#include "hostfxr_dotnet_runtime.h"

#include "../../utils/dirs_utils.h"
#include "hostfxr_dotnet_config.h"

#include "core/io/file_access.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "../../utils/semver.h"
#include "hostfxr_resolver.h"

#include "core/io/dir_access.h"
#endif

namespace DotNet {

hostfxr_initialize_for_dotnet_command_line_fn hostfxr_initialize_for_dotnet_command_line = nullptr;
hostfxr_initialize_for_runtime_config_fn hostfxr_initialize_for_runtime_config = nullptr;
hostfxr_get_runtime_delegate_fn hostfxr_get_runtime_delegate = nullptr;
hostfxr_close_fn hostfxr_close = nullptr;

#ifdef _WIN32
static_assert(sizeof(char_t) == sizeof(char16_t));
using HostFxrCharString = Char16String;
#define HOSTFXR_STR(m_str) L##m_str
#else
static_assert(sizeof(char_t) == sizeof(char));
using HostFxrCharString = CharString;
#define HOSTFXR_STR(m_str) m_str
#endif

HostFxrCharString str_to_hostfxr(const String &p_str) {
#ifdef _WIN32
	return p_str.utf16();
#else
	return p_str.utf8();
#endif
}

const char_t *get_data(const HostFxrCharString &p_char_str) {
	return (const char_t *)p_char_str.get_data();
}

#ifdef TOOLS_ENABLED
typedef bool(CORECLR_DELEGATE_CALLTYPE *godot_plugin_load_assembly_fn)(
		const char_t *p_assembly_name,
		const char_t *p_fully_qualified_type_name,
		const char_t *p_method_name,
		GDExtensionInitializationFunction *r_initialization_function);

typedef bool(CORECLR_DELEGATE_CALLTYPE *godot_plugin_unload_assembly_fn)(const char_t *p_assembly_name);

godot_plugin_load_assembly_fn godot_plugin_load_assembly = nullptr;
godot_plugin_unload_assembly_fn godot_plugin_unload_assembly = nullptr;
#endif // TOOLS_ENABLED

bool HostFxrDotNetRuntime::try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const {
	String assembly_path = p_assemblies_directory.path_join(p_assembly_name + ".dll");
	String runtime_config_path = p_assemblies_directory.path_join(p_assembly_name + ".runtimeconfig.json");

	if (!FileAccess::exists(assembly_path)) {
		return false;
	}

	Ref<HostFxrDotNetConfig> config;
	config.instantiate();
	config->assembly_name = p_assembly_name;
	config->assembly_path = assembly_path;
	config->runtime_config_path = runtime_config_path;
	// TODO: Assume the entry-point is in a 'Main' class under a namespace that matches the assembly name,
	// and the method is named 'Init'. In the future we may want to provide a way to customize the entry-point
	// in project settings.
	config->entry_point_class_name = p_assembly_name + ".Main";
	config->entry_point_method_name = "Init";

	r_extension_config = config;
	return true;
}

bool HostFxrDotNetRuntime::try_initialize() {
	if (hostfxr_dll_handle != nullptr) {
		// Already initialized.
		return true;
	}

	bool success = try_load_hostfxr(hostfxr_dll_handle);
#ifdef TOOLS_ENABLED
	success = success && try_load_godot_plugin_loader_assembly();
#endif
	return success;
}

bool HostFxrDotNetRuntime::try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
#ifdef TOOLS_ENABLED
	// In the editor we use the Godot.PluginLoader to load .NET assemblies
	// so this probing is unnecessary, it will always be successful.
	return true;
#else
	const Ref<HostFxrDotNetConfig> config = p_extension_config;
	if (config.is_null()) {
		return false;
	}

	// Exported projects are always self-contained.
	{
		print_verbose(".NET: Loading " + config->assembly_name + " as self-contained.");
		load_assembly_and_get_function_pointer = initialize_hostfxr_self_contained(get_data(str_to_hostfxr(config->assembly_path)));
	}

	return load_assembly_and_get_function_pointer != nullptr;
#endif // TOOLS_ENABLED
}

GDExtensionInitializationFunction HostFxrDotNetRuntime::get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
#ifdef TOOLS_ENABLED
	ERR_FAIL_NULL_V_MSG(godot_plugin_load_assembly, nullptr, ".NET: Godot.PluginLoader is not initialized.");
#else
	ERR_FAIL_NULL_V_MSG(load_assembly_and_get_function_pointer, nullptr, ".NET: hostfxr is not initialized.");
#endif
	const Ref<HostFxrDotNetConfig> config = p_extension_config;
	ERR_FAIL_COND_V_MSG(config.is_null(), nullptr, ".NET: Extension configuration is incompatible with hostfxr.");

	GDExtensionInitializationFunction entry_point = nullptr;

	String type_name = config->entry_point_class_name + ", " + config->assembly_name;
	String method_name = config->entry_point_method_name;

#ifdef TOOLS_ENABLED
	bool success = godot_plugin_load_assembly(
			get_data(str_to_hostfxr(config->assembly_path)),
			get_data(str_to_hostfxr(type_name)),
			get_data(str_to_hostfxr(method_name)),
			&entry_point);
	ERR_FAIL_COND_V_MSG(!success, nullptr, vformat(".NET: Failed to load assembly '%s'.", config->assembly_name));
	ERR_FAIL_NULL_V_MSG(entry_point, nullptr, ".NET: Failed to get entry-point function pointer.");
	loaded_extension_configs.push_back(config);
#else
	int rc = load_assembly_and_get_function_pointer(
			get_data(str_to_hostfxr(config->assembly_path)),
			get_data(str_to_hostfxr(type_name)),
			get_data(str_to_hostfxr(method_name)),
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			(void **)&entry_point);
	ERR_FAIL_COND_V_MSG(rc != 0, nullptr, ".NET: Failed to get entry-point function pointer with code: " + itos(rc));
#endif

	return entry_point;
}

void HostFxrDotNetRuntime::close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
#ifdef TOOLS_ENABLED
	ERR_FAIL_NULL_MSG(godot_plugin_unload_assembly, ".NET: Godot.PluginLoader not initialized.");
	const Ref<HostFxrDotNetConfig> config = p_extension_config;
	ERR_FAIL_COND_MSG(config.is_null(), ".NET: Extension configuration is incompatible with hostfxr.");

	if (config->assembly_name == "Godot.EditorIntegration") {
		// Never unload the Godot.EditorIntegration assembly.
		return;
	}

	bool success = godot_plugin_unload_assembly(get_data(str_to_hostfxr(config->assembly_path)));
	ERR_FAIL_COND_MSG(!success, vformat(".NET: Failed to unload assembly '%s'.", config->assembly_name));
	loaded_extension_configs.erase(p_extension_config);
#else
	// In an exported game we don't use Godot.PluginLoader to load assemblies
	// so we can't unload extensions. But that's fine because there shouldn't
	// be any assembly unloading/reloading in an exported game.
#endif // TOOLS_ENABLED
}

#ifdef TOOLS_ENABLED
bool HostFxrDotNetRuntime::try_get_dotnet_root_from_command_line(String &r_dotnet_root) {
	String pipe;
	List<String> args;
	args.push_back("--list-sdks");

	int exitcode;
	Error err = OS::get_singleton()->execute("dotnet", args, &pipe, &exitcode, true);

	ERR_FAIL_COND_V_MSG(err != OK, false, ".NET failed to get list of installed SDKs. Error: " + String(error_names[err]));
	if (exitcode != 0) {
		// Print the process output because it may contain more details about the error.
		print_line(pipe);
		ERR_FAIL_V_MSG(false, ".NET failed to get list of installed SDKs. See output above for more details.");
	}

	Vector<String> sdks = pipe.strip_edges().replace("\r\n", "\n").split("\n", false);

	SemVer latest_sdk_version;
	String latest_sdk_path;

	for (const String &sdk : sdks) {
		// The format of the SDK lines is:
		// 8.0.401 [/usr/share/dotnet/sdk]
		String version_string = sdk.get_slicec(' ', 0);
		String path = sdk.get_slicec(' ', 1);
		path = path.substr(1, path.length() - 2);

		SemVer version;
		if (!SemVer::try_parse(version_string, version)) {
			WARN_PRINT("Unable to parse .NET SDK version '" + version_string + "'.");
			continue;
		}

		if (!DirAccess::exists(path)) {
			WARN_PRINT("Found .NET SDK version '" + version_string + "' with invalid path '" + path + "'.");
			continue;
		}

		if (version > latest_sdk_version) {
			latest_sdk_version = version;
			latest_sdk_path = path;
		}
	}

	if (!latest_sdk_path.is_empty()) {
		print_verbose("Found .NET SDK at " + latest_sdk_path);
		// The dotnet root is the parent directory.
		r_dotnet_root = latest_sdk_path.path_join("..").simplify_path();
		return true;
	}

	return false;
}
#endif

String HostFxrDotNetRuntime::find_hostfxr() {
#ifdef TOOLS_ENABLED

	String dotnet_root;
	String hostfxr_path;

	if (HostFxrResolver::try_get_path(dotnet_root, hostfxr_path)) {
		return hostfxr_path;
	}

	// hostfxr_resolver doesn't look for dotnet in PATH. If it fails, we try to use the dotnet
	// executable in PATH to find the dotnet root and get the hostfxr path from there.
	if (try_get_dotnet_root_from_command_line(dotnet_root)) {
		if (HostFxrResolver::try_get_path_from_dotnet_root(dotnet_root, hostfxr_path)) {
			return hostfxr_path;
		}
	}

	ERR_PRINT(String() + ".NET: One of the dependent libraries is missing. " +
			"Typically when the `hostfxr`, `hostpolicy` or `coreclr` dynamic " +
			"libraries are not present in the expected locations.");

	return String();

#else

#if defined(WINDOWS_ENABLED)
	String hostfxr_path = Dirs::get_project_assemblies_path().path_join("hostfxr.dll");
#elif defined(MACOS_ENABLED)
	String hostfxr_path = Dirs::get_project_assemblies_path().path_join("libhostfxr.dylib");
#elif defined(UNIX_ENABLED)
	String hostfxr_path = Dirs::get_project_assemblies_path().path_join("libhostfxr.so");
#else
#error "Platform not supported (yet?)"
#endif

	if (FileAccess::exists(hostfxr_path)) {
		return hostfxr_path;
	}

	return String();

#endif
}

bool HostFxrDotNetRuntime::try_load_hostfxr(void *&r_hostfxr_dll_handle) {
	String hostfxr_path = find_hostfxr();

	if (hostfxr_path.is_empty()) {
		return false;
	}

	print_verbose("Found hostfxr: " + hostfxr_path);

	Error err = OS::get_singleton()->open_dynamic_library(hostfxr_path, r_hostfxr_dll_handle);

	if (err != OK) {
		return false;
	}

	void *lib = r_hostfxr_dll_handle;

	void *symbol = nullptr;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "hostfxr_initialize_for_dotnet_command_line", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	hostfxr_initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "hostfxr_initialize_for_runtime_config", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	hostfxr_initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "hostfxr_get_runtime_delegate", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	hostfxr_get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)symbol;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib, "hostfxr_close", symbol);
	ERR_FAIL_COND_V(err != OK, false);
	hostfxr_close = (hostfxr_close_fn)symbol;

	return (hostfxr_initialize_for_dotnet_command_line &&
			hostfxr_initialize_for_runtime_config &&
			hostfxr_get_runtime_delegate &&
			hostfxr_close);
}

#ifdef TOOLS_ENABLED
bool HostFxrDotNetRuntime::try_load_godot_plugin_loader_assembly() {
	if (godot_plugin_load_assembly && godot_plugin_unload_assembly) {
		// Godot.PluginLoader already initialized.
		return true;
	}

	String assembly_name = "Godot.PluginLoader";
	String assemblies_path = Dirs::get_editor_assemblies_path().path_join("Godot.PluginLoader");

	String plugin_loader_assembly_path = assemblies_path.path_join(assembly_name + ".dll");
	String plugin_loader_runtime_config_path = assemblies_path.path_join(assembly_name + ".runtimeconfig.json");

	// Godot.PluginLoader is always framework-dependent.
	{
		print_verbose(".NET: Loading " + assembly_name + " using runtimeconfig.");
		load_assembly_and_get_function_pointer = initialize_hostfxr_for_runtime_config(get_data(str_to_hostfxr(plugin_loader_runtime_config_path)));
	}

	String type_name = "Godot.PluginLoader.Main";
	String method_name = "LoadAssembly";

	int rc = load_assembly_and_get_function_pointer(get_data(str_to_hostfxr(plugin_loader_assembly_path)),
			get_data(str_to_hostfxr(type_name + ", " + assembly_name)),
			get_data(str_to_hostfxr(method_name)),
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			(void **)&godot_plugin_load_assembly);
	ERR_FAIL_COND_V_MSG(rc != 0, false, ".NET: Failed to get '" + type_name + "." + method_name + "' function pointer with code: " + itos(rc));

	method_name = "UnloadAssembly";

	rc = load_assembly_and_get_function_pointer(get_data(str_to_hostfxr(plugin_loader_assembly_path)),
			get_data(str_to_hostfxr(type_name + ", " + assembly_name)),
			get_data(str_to_hostfxr(method_name)),
			UNMANAGEDCALLERSONLY_METHOD,
			nullptr,
			(void **)&godot_plugin_unload_assembly);
	ERR_FAIL_COND_V_MSG(rc != 0, false, ".NET: Failed to get '" + type_name + "." + method_name + "' function pointer with code: " + itos(rc));

	return (godot_plugin_load_assembly && godot_plugin_unload_assembly);
}
#endif // TOOLS_ENABLED

load_assembly_and_get_function_pointer_fn HostFxrDotNetRuntime::initialize_hostfxr_for_runtime_config(const char_t *p_runtime_config_path) {
	hostfxr_handle cxt = nullptr;

	int rc = hostfxr_initialize_for_runtime_config(p_runtime_config_path, nullptr, &cxt);
	if (rc != 0 || cxt == nullptr) {
		if (rc == 1) {
			print_verbose("hostfxr was already initialized, re-using existing host context.");
		} else if (rc == 2) {
			print_verbose("hostfxr was already initialized with different runtime properties, re-using existing host context.");
		} else {
			hostfxr_close(cxt);
			ERR_FAIL_V_MSG(nullptr, "hostfxr_initialize_for_runtime_config failed with code: " + itos(rc));
		}
	}

	void *delegate = nullptr;

	rc = hostfxr_get_runtime_delegate(cxt,
			hdt_load_assembly_and_get_function_pointer, &delegate);
	if (rc != 0 || delegate == nullptr) {
		ERR_FAIL_V_MSG(nullptr, "hostfxr_get_runtime_delegate failed with code: " + itos(rc));
	}

	hostfxr_close(cxt);

	return (load_assembly_and_get_function_pointer_fn)delegate;
}

load_assembly_and_get_function_pointer_fn HostFxrDotNetRuntime::initialize_hostfxr_self_contained(const char_t *p_main_assembly_path) {
	hostfxr_handle cxt = nullptr;

	List<String> cmdline_args = OS::get_singleton()->get_cmdline_args();

	List<HostFxrCharString> argv_store;
	LocalVector<const char_t *> argv;
	argv.resize(cmdline_args.size() + 1);

	argv[0] = p_main_assembly_path;

	int i = 1;
	for (const String &arg : cmdline_args) {
		HostFxrCharString &stored = argv_store.push_back(str_to_hostfxr(arg))->get();
		argv[i] = get_data(stored);
		i++;
	}

	int rc = hostfxr_initialize_for_dotnet_command_line(argv.size(), argv.ptr(), nullptr, &cxt);
	if (rc != 0 || cxt == nullptr) {
		hostfxr_close(cxt);
		ERR_FAIL_V_MSG(nullptr, "hostfxr_initialize_for_dotnet_command_line failed with code: " + itos(rc));
	}

	void *delegate = nullptr;

	rc = hostfxr_get_runtime_delegate(cxt,
			hdt_load_assembly_and_get_function_pointer, &delegate);
	if (rc != 0 || delegate == nullptr) {
		ERR_FAIL_V_MSG(nullptr, "hostfxr_get_runtime_delegate failed with code: " + itos(rc));
	}

	hostfxr_close(cxt);

	return (load_assembly_and_get_function_pointer_fn)delegate;
}

HostFxrDotNetRuntime::~HostFxrDotNetRuntime() {
#ifdef TOOLS_ENABLED
	for (const Ref<GDExtensionDotNetConfig> &config : loaded_extension_configs) {
		close_extension(config);
	}
#endif

	if (hostfxr_dll_handle != nullptr) {
		OS::get_singleton()->close_dynamic_library(hostfxr_dll_handle);
	}
}

} // namespace DotNet

#endif // DOTNET_HOSTFXR_ENABLED
