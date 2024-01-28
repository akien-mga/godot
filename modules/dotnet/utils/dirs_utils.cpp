/**************************************************************************/
/*  dirs_utils.cpp                                                        */
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

#include "dirs_utils.h"

#include "path_utils.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "core/version.h"
#include "editor/file_system/editor_paths.h"
#endif

namespace DotNet {
namespace Dirs {

class _Dirs {
public:
#ifdef TOOLS_ENABLED
	String editor_assemblies_path;
#endif

	String project_assemblies_path;

private:
	_Dirs() {
		String exe_path = OS::get_singleton()->get_executable_path().get_base_dir();

#ifdef TOOLS_ENABLED

		// Version in the format "4.3.0[-dev]" that will be used as the subdirectory that contains the
		// editor assemblies for this specific version of the editor.
		String version = _MKSTR(GODOT_VERSION_MAJOR) "." _MKSTR(GODOT_VERSION_MINOR) "." _MKSTR(GODOT_VERSION_PATCH);
		if (GODOT_VERSION_STATUS) {
			version += "-" GODOT_VERSION_STATUS;
		}

		project_assemblies_path = get_project_output_path(get_project_csproj_path());
		editor_assemblies_path = exe_path.path_join("Godot.NET").path_join(version);

#ifdef MACOS_ENABLED
		// On macOS, use directory outside .app bundle, since .app bundle is read-only.
		editor_assemblies_path = EditorPaths::get_singleton()->get_data_dir().path_join("Godot.NET").path_join(version);
#endif

#else // TOOLS_ENABLED

		String platform = OS::get_singleton()->get_name();
		String arch = Engine::get_singleton()->get_architecture_name();
		String data_dir_name = get_exported_data_directory_name(get_project_assembly_name(), platform, arch);

		String packed_path = "res://.godot/dotnet/" + arch;

		String data_dir_path;
		if (DirAccess::exists(packed_path)) {
			// The dotnet publish data is packed in the pck/zip.
			data_dir_path = packed_path;
		} else {
			// The dotnet publish data is in a directory next to the executable.
			data_dir_path = exe_path.path_join(data_dir_name);

#ifdef MACOS_ENABLED
			if (!DirAccess::exists(data_dir_path)) {
				String macos_bundle_dir = OS::get_singleton()->get_bundle_resource_dir();
				data_dir_path = macos_bundle_dir.path_join(data_dir_name);
			}
#endif
		}

		project_assemblies_path = data_dir_path;

#endif // TOOLS_ENABLED
	}

public:
	static _Dirs &get_singleton() {
		static _Dirs singleton;
		return singleton;
	}
};

String get_exported_data_directory_name(const String &p_assembly_name, const String &p_platform, const String &p_architecture) {
	return "data_" + p_assembly_name + "_" + p_platform + "_" + p_architecture;
}

#ifdef TOOLS_ENABLED
String get_editor_assemblies_path() {
	return _Dirs::get_singleton().editor_assemblies_path;
}

String get_project_output_path(const String &p_project_path) {
	if (!FileAccess::exists(p_project_path)) {
		return String();
	}

	String pipe;
	List<String> args;
	args.push_back("build");
	args.push_back(p_project_path);
	args.push_back("--getProperty:OutputPath");

	int exitcode;
	Error err = OS::get_singleton()->execute("dotnet", args, &pipe, &exitcode, true);

	ERR_FAIL_COND_V_MSG(err != OK, String(), ".NET failed to get output path for '" + p_project_path + "'. Error: " + error_names[err]);
	if (exitcode != 0) {
		// Print the process output because it may contain more details about the error.
		print_line(pipe);
		ERR_FAIL_V_MSG(String(), ".NET failed to get output path for '" + p_project_path + "'. See output above for more details.");
	}

	String output_path = pipe.strip_edges();
	if (output_path.is_relative_path()) {
		// If the output path is relative, then it must be relative to the project file.
		output_path = p_project_path.path_join(output_path);
	}
	return output_path;
}
#endif

String get_project_assemblies_path() {
	if (_Dirs::get_singleton().project_assemblies_path.is_empty()) {
#ifdef TOOLS_ENABLED
		_Dirs::get_singleton().project_assemblies_path = Dirs::get_project_output_path(Dirs::get_project_csproj_path());
#else
		ERR_FAIL_V_MSG(String(), "Failed to get project assemblies path.");
#endif
	}
	return _Dirs::get_singleton().project_assemblies_path;
}

String get_project_sln_path() {
	String slnParentDir = ProjectSettings::get_singleton()->get_setting("dotnet/project/solution_directory", "");

	if (slnParentDir.is_empty()) {
		slnParentDir = "res://";
	} else if (!slnParentDir.begins_with("res://")) {
		slnParentDir = "res://" + slnParentDir;
	}

	slnParentDir = ProjectSettings::get_singleton()->globalize_path(slnParentDir);

	return slnParentDir.path_join(get_project_assembly_name() + ".sln");
}

String get_project_csproj_path() {
	// The .csproj file is always in the same directory as the `project.godot` file.
	return ProjectSettings::get_singleton()->globalize_path("res://" + get_project_assembly_name() + ".csproj");
}

String get_project_assembly_name() {
	String name = GLOBAL_GET("dotnet/project/assembly_name");
	if (name.is_empty()) {
		name = GLOBAL_GET("application/config/name");
		name = name.strip_edges();
		static LocalVector<String> invalid_chars = {
			// Windows reserved filename chars.
			":", "*", "?", "\"", "<", ">", "|",
			// Directory separators.
			"/", "\\",
			// Other chars that have been found to break assembly loading.
			";", "'", "=", ","
		};
		for (uint32_t i = 0; i < invalid_chars.size(); i++) {
			name = name.replace(invalid_chars[i], "-");
		}
	}

	if (name.is_empty()) {
		name = "UnnamedProject";
	}

	return name;
}

} // namespace Dirs
} // namespace DotNet
