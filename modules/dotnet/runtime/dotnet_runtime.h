/**************************************************************************/
/*  dotnet_runtime.h                                                      */
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

#include "../extension/gdextension_dotnet_config.h"

#include "core/object/ref_counted.h"

namespace DotNet {

class DotNetRuntime : public RefCounted {
public:
	/**
	 * Get the display name for these .NET runtime hosting APIs.
	 */
	virtual String get_name() const = 0;

	/**
	 * Indicates if these hosting APIs support reloading assemblies.
	 */
	virtual bool supports_reloading() const { return false; }

	/**
	 * Try to get the configuration for the .NET extension with the given assembly
	 * name from the given directory.
	 * This may fail if the .NET extension doesn't support this .NET runtime.
	 */
	virtual bool try_get_extension_config(const String &p_assembly_name, const String &p_assemblies_directory, Ref<GDExtensionDotNetConfig> &r_extension_config) const = 0;

	/**
	 * Try to initialize the .NET runtime using these hosting APIs.
	 */
	virtual bool try_initialize() = 0;

	/**
	 * Try to open the requested .NET extension using the .NET runtime that
	 * was previously initialized with these hosting APIs.
	 * This may fail if the .NET extension doesn't support this .NET runtime.
	 */
	virtual bool try_open_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) = 0;

	/**
	 * Get a function pointer to the .NET extension's entry-point.
	 * The .NET runtime must be initialized and the .NET extension open.
	 */
	virtual GDExtensionInitializationFunction get_extension_entry_point(const Ref<GDExtensionDotNetConfig> &p_extension_config) = 0;

	/**
	 * Close the .NET extension that has been previously opened using the .NET
	 * runtime with these hosting APIs.
	 */
	virtual void close_extension(const Ref<GDExtensionDotNetConfig> &p_extension_config) = 0;

	virtual ~DotNetRuntime() {}
};

} // namespace DotNet
