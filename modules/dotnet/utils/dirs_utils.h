/**************************************************************************/
/*  dirs_utils.h                                                          */
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

#include "core/string/ustring.h"

namespace DotNet {
namespace Dirs {

String get_exported_data_directory_name(const String &p_assembly_name, const String &p_platform, const String &p_architecture);

#ifdef TOOLS_ENABLED
/**
 * Retrieves the path to the editor assemblies.
 * Editor assemblies are downloaded on-demand when the project uses .NET/C#
 * and stored in this path.
 */
String get_editor_assemblies_path();
#endif

/**
 * Retrieves the path to the user project assemblies.
 * For a project running from the editor this is inside the project data path and
 * matches `get_project_output_path` for `get_project_csproj_path`.
 * For an exported project this is a directory next to the executable or inside
 * the PCK.
 */
String get_project_assemblies_path();

#ifdef TOOLS_ENABLED
/**
 * Retrieves the output path for the given C# project.
 * This is the path that contains the build output (assemblies).
 */
String get_project_output_path(const String &p_project_path);

/**
 * Retrieves the path to the user project's solution file.
 * This is the solution file that is used to open the main project in external editors.
 */
String get_project_sln_path();

/**
 * Retrieves the path to user project's csproj file.
 * This is the C# project file that is used to build the main project's assembly.
 */
String get_project_csproj_path();
#endif

/**
 * Retrieves the name of the user project assembly.
 * Usually the same as the appname but can be a different name if it contains
 * disallowed characters.
 */
String get_project_assembly_name();

} // namespace Dirs
} // namespace DotNet
