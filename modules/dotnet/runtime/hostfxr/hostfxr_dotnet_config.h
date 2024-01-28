/**************************************************************************/
/*  hostfxr_dotnet_config.h                                               */
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

#include "../../extension/gdextension_dotnet_config.h"

#include "core/string/ustring.h"

namespace DotNet {

class HostFxrDotNetConfig : public GDExtensionDotNetConfig {
	GDSOFTCLASS(HostFxrDotNetConfig, GDExtensionDotNetConfig)
	friend class HostFxrDotNetRuntime;

private:
	/**
	 * Name of the .NET assembly.
	 */
	String assembly_name;

	/**
	 * Path to the .NET assembly DLL.
	 */
	String assembly_path;

	/**
	 * Path to the runtimeconfig.json file (if it exists).
	 */
	String runtime_config_path;

	/**
	 * Name of the class that contains the entry-point.
	 * The name must be fully-qualified with namespaces.
	 */
	String entry_point_class_name;

	/**
	 * Name of the method to be used as the entry-point.
	 */
	String entry_point_method_name;

public:
	String get_path() const override { return assembly_path; }
	String get_entry_point() const override { return entry_point_class_name + "::" + entry_point_method_name; }
};

} // namespace DotNet

#endif // DOTNET_HOSTFXR_ENABLED
