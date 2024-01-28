/**************************************************************************/
/*  nativeaot_dotnet_runtime.cpp                                          */
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

#ifdef DOTNET_NATIVEAOT_ENABLED

#include "nativeaot_dotnet_runtime.h"

#include "nativeaot_dotnet_config.h"

#include "core/os/os.h"

namespace DotNet {

bool NativeAOTDotNetRuntime::try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const {
#if defined(WINDOWS_ENABLED)
	String lib_path = p_assemblies_directory.path_join(p_assembly_name + ".dll");
#elif defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	String lib_path = p_assemblies_directory.path_join(p_assembly_name + ".dylib");
#elif defined(UNIX_ENABLED)
	String lib_path = p_assemblies_directory.path_join(p_assembly_name + ".so");
#else
#error "Platform not supported (yet?)"
#endif

	Ref<NativeAOTDotNetConfig> config;
	config.instantiate();
	config->lib_path = lib_path;
	// TODO: Assume the entry-point is exposed as 'init'. In the future we may
	// want to provide a way to customize the entry-point in project settings.
	config->entry_point = "init";

	r_extension_config = config;
	return true;
}

bool NativeAOTDotNetRuntime::try_initialize() {
	// The NativeAOT runtime is initialized on opening the .NET extension.
	return true;
}

bool NativeAOTDotNetRuntime::try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	const Ref<NativeAOTDotNetConfig> config = p_extension_config;
	if (config.is_null()) {
		return false;
	}

	Error err = OS::get_singleton()->open_dynamic_library(config->lib_path, lib_handle);

	return err == OK;
}

GDExtensionInitializationFunction NativeAOTDotNetRuntime::get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	ERR_FAIL_NULL_V_MSG(lib_handle, nullptr, ".NET: NativeAOT is not initialized.");
	const Ref<NativeAOTDotNetConfig> config = p_extension_config;
	ERR_FAIL_COND_V_MSG(config.is_null(), nullptr, ".NET: Extension configuration is incompatible with NativeAOT.");

	GDExtensionInitializationFunction entry_point = nullptr;

	void *symbol;
	Error err = OS::get_singleton()->get_dynamic_library_symbol_handle(lib_handle, config->entry_point, symbol);
	ERR_FAIL_COND_V_MSG(err != OK, nullptr, ".NET: Failed to get entry-point function pointer.");

	entry_point = (GDExtensionInitializationFunction)symbol;
	return entry_point;
}

void NativeAOTDotNetRuntime::close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) {
	// NativeAOT does not support unloading.
	// See: https://github.com/dotnet/runtime/issues/64629
}

} // namespace DotNet

#endif // DOTNET_NATIVEAOT_ENABLED
