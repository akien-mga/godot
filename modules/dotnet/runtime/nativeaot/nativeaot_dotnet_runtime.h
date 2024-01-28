/**************************************************************************/
/*  nativeaot_dotnet_runtime.h                                            */
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

#ifdef DOTNET_NATIVEAOT_ENABLED

#include "../dotnet_runtime.h"

namespace DotNet {

class NativeAOTDotNetRuntime : public DotNetRuntime {
private:
	void *lib_handle = nullptr;

public:
	String get_name() const override { return "NativeAOT"; }

	bool try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const override;

	bool try_initialize() override;
	bool try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;
	GDExtensionInitializationFunction get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;
	void close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) override;
};

} // namespace DotNet

#endif // DOTNET_NATIVEAOT_ENABLED
