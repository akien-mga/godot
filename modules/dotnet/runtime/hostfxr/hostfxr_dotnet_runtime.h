/**************************************************************************/
/*  hostfxr_dotnet_runtime.h                                              */
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

#pragma once

#ifdef DOTNET_HOSTFXR_ENABLED

#include "../dotnet_runtime.h"

#include "../../thirdparty/coreclr_delegates.h"
#include "../../thirdparty/hostfxr.h"

namespace DotNet {

class HostFxrDotNetRuntime : public DotNetRuntime {
private:
	void *hostfxr_dll_handle = nullptr;
	load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;

#ifdef TOOLS_ENABLED
	LocalVector<Ref<GDExtensionDotNetConfig>> loaded_extension_configs;
#endif

public:
	String get_name() const override { return "Host FXR"; }

	bool supports_reloading() const override {
#ifdef TOOLS_ENABLED
		return true;
#else
		return false;
#endif
	}

	bool try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const override;

	bool try_initialize() override;
	bool try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;
	GDExtensionInitializationFunction get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;
	void close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;

private:
	bool try_get_dotnet_root_from_command_line(String &r_dotnet_root);
	String find_hostfxr();
	bool try_load_hostfxr(void *&r_hostfxr_dll_handle);

#ifdef TOOLS_ENABLED
	bool try_load_godot_plugin_loader_assembly();
#endif

	load_assembly_and_get_function_pointer_fn initialize_hostfxr_for_runtime_config(const char_t *p_config_path);
	load_assembly_and_get_function_pointer_fn initialize_hostfxr_self_contained(const char_t *p_main_assembly_path);

public:
	~HostFxrDotNetRuntime();
};

} // namespace DotNet

#endif // DOTNET_HOSTFXR_ENABLED
