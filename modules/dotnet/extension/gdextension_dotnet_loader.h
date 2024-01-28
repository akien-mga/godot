/**************************************************************************/
/*  gdextension_dotnet_loader.h                                           */
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

#include "core/extension/gdextension_loader.h"

namespace DotNet {

class DotNetRuntime;
class GDExtensionDotNetConfig;

class GDExtensionDotNetLoader : public GDExtensionLoader {
	GDSOFTCLASS(GDExtensionDotNetLoader, GDExtensionLoader)

private:
	const Ref<DotNetRuntime> runtime;
	const Ref<GDExtensionDotNetConfig> config;

#ifdef TOOLS_ENABLED
	uint64_t assembly_last_modified_time = 0;
#endif

public:
	const Ref<GDExtensionDotNetConfig> get_config() const { return config; }

	Error open_library(const String &p_path) override;
	Error initialize(GDExtensionInterfaceGetProcAddress p_get_proc_address, const Ref<GDExtension> &p_extension, GDExtensionInitialization *r_initialization) override;
	void close_library() override;

	bool is_library_open() const override { return true; }

	bool has_library_changed() const override;

	bool library_exists() const override;

	GDExtensionDotNetLoader(const Ref<DotNetRuntime> &p_runtime, const Ref<GDExtensionDotNetConfig> &p_config) :
			runtime(p_runtime),
			config(p_config) {
	}
};

} // namespace DotNet
