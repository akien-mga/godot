/**************************************************************************/
/*  dotnet_runtime_manager.h                                              */
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

#include "dotnet_runtime.h"

namespace DotNet {

class DotNetRuntimeManager {
private:
	static DotNetRuntimeManager *singleton;
	LocalVector<Ref<DotNetRuntime>> runtimes;

	/**
	 * Get a loaded .NET GDExtension by its assembly name and the assemblies directory it was loaded from.
	 * The assemblies directory is optional, if not specified, any extension matching the assembly name will
	 * be returned.
	 */
	const Ref<GDExtension> get_loaded_extension(const String &p_assembly_name, const String &p_assemblies_directory = String()) const;

public:
	static DotNetRuntimeManager *get_singleton();

	bool is_assembly_loaded(const String &p_assembly_name, const String &p_assemblies_directory = String()) const;

	bool try_load_extension(const String &p_assembly_name, const String &p_assemblies_directory);
	bool try_reload_extension(const String &p_assembly_name, const String &p_assemblies_directory = String());
	bool try_unload_extension(const String &p_assembly_name, const String &p_assemblies_directory = String());

	DotNetRuntimeManager();
	~DotNetRuntimeManager();
};

} // namespace DotNet
