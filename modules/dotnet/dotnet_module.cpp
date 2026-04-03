/**************************************************************************/
/*  dotnet_module.cpp                                                     */
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

#include "dotnet_module.h"

#include "utils/dirs_utils.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "editor/RestoreEditorPackages.proj.gen.h"
#include "editor/dotnet_status_indicator.h"
// #include "editor/welcome_dialog.h"
#include "runtime/hostfxr/hostfxr_dotnet_runtime.h"

#include "core/config/engine.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"
#endif

namespace DotNet {

DotNetModule *DotNetModule::singleton = nullptr;

DotNetRuntimeManager *DotNetModule::runtime_manager = nullptr;

DotNetModule *DotNetModule::get_singleton() {
	return singleton;
}

bool DotNetModule::should_initialize() {
#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_project_manager_hint()) {
		// Never initialize the module in the project manager.
		return false;
	}

	// TODO(@raulsntos): See `request_enable_dotnet_features` for how we should handle this in the future once we have editor unification implemented. For now, we'll just always initialize the module in the editor.

	// If we can find a C# project or solution in the workspace,
	// assume the Godot project uses C# and needs to initialize the module.
	// return FileAccess::exists(Dirs::get_project_csproj_path()) || FileAccess::exists(Dirs::get_project_sln_path());
	// TODO(@raulsntos): Actually, we always initialize the module since we haven't implemented editor unification yet.
	return true;
#else
	// The exported project was built with .NET support,
	// so it needs to initialize the module.
	return OS::get_singleton()->has_feature("dotnet");
#endif
}

void DotNetModule::initialize() {
	print_verbose(".NET: Initializing module.");

	_update_initialization_state(InitState::INITIALIZING);

	// When initializing for the first time, create the runtime manager.
	// If initialization fails and is retried, the runtime manager should already exist.
	if (runtime_manager == nullptr) {
		runtime_manager = memnew(DotNetRuntimeManager);
	}
	CRASH_COND(runtime_manager == nullptr);

	// There are 3 possible cases here:
	// 1. Exported project.
	//    Doesn't need the editor integration, it loads assemblies without 'Godot.PluginLoader'.
	// 2. Editor build running outside the editor.
	//    Needs the 'Godot.PluginLoader' assembly restored from the editor packages,
	//    but should not load the 'Godot.EditorIntegration' assembly.
	// 3. Editor build running inside the editor.
	//    Needs the 'Godot.PluginLoader' assembly restored from the editor packages,
	//    and should load the 'Godot.EditorIntegration' assembly.
#ifdef TOOLS_ENABLED
	String dotnet_sdk_path;
	if (!HostFxrDotNetRuntime::try_find_dotnet_sdk(dotnet_sdk_path)) {
		_set_initialization_failed(FailedState::DOTNET_SDK_NOT_FOUND, TTR("Could not find .NET SDK. Please install from https://get.dot.net"));
		return;
	}

	print_verbose(vformat(".NET: .NET SDK installation found at '%s'.", dotnet_sdk_path));

	if (Engine::get_singleton()->is_editor_hint()) {
		const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
		if (!try_restore_editor_packages(editor_assemblies_dir)) {
			_set_initialization_failed(FailedState::EDITOR_INTEGRATION_RESTORE_FAILED, TTR("Failed to restore .NET editor integration. Check the console output for more details."));
			return;
		}
		if (!runtime_manager->try_load_extension("Godot.EditorIntegration", editor_assemblies_dir)) {
			_set_initialization_failed(FailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE, TTR("Failed to load .NET editor integration. Check the console output for more details."));
			return;
		}
	}
#endif

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	bool should_load_project_assembly = true;

#ifdef TOOLS_ENABLED
	// Listen for changes to the assembly name setting so we can update the loaded assembly accordingly.
	ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &DotNetModule::_on_project_settings_changed));

	fs_watcher.instantiate();
	fs_watcher->set_path(assemblies_dir.path_join(assembly_name + ".dll"));
	fs_watcher->callable = callable_mp(this, &DotNetModule::_on_project_assembly_changed);
	callable_mp(this, &DotNetModule::_start_fs_watcher).call_deferred();

	should_load_project_assembly = FileAccess::exists(fs_watcher->get_path());
	if (!should_load_project_assembly) {
		// DLL doesn't exist yet. Determine whether a .csproj is available.
		if (FileAccess::exists(Dirs::get_project_csproj_path())) {
			// The .csproj exists, so it was just not built yet.
			_set_user_assembly_load_failed(UserAssemblyState::DLL_NOT_FOUND);
		} else {
			// There is no .csproj, so this project may not use C#.
			_set_user_assembly_load_failed(UserAssemblyState::PROJECT_NOT_FOUND);
		}
	}
#endif

	if (should_load_project_assembly) {
		if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
			WARN_PRINT(".NET: Failed to load assembly '" + assembly_name + "'.");
#ifdef TOOLS_ENABLED
			_set_user_assembly_load_failed(UserAssemblyState::FAILED_TO_LOAD);
		} else {
			_set_user_assembly_load_success(assembly_name);
#endif
		}
	}

	// In editor builds, the initialization has to wait for the editor integration which will
	// set the state to INITIALIZED once it's ready or FAILED if something goes wrong.
	// In exported builds, we can set the state to INITIALIZED right away since we don't have the editor integration.
#ifndef TOOLS_ENABLED
	_update_initialization_state(InitState::INITIALIZED);
#endif

	print_verbose(".NET: Module initialized.");
}

#ifdef TOOLS_ENABLED
void DotNetModule::_start_init_timer() {
	Ref<SceneTreeTimer> timer = SceneTree::get_singleton()->create_timer(5.0);
	timer->connect(SNAME("timeout"), callable_mp(this, &DotNetModule::_on_init_timeout));
}

void DotNetModule::_on_init_timeout() {
	if (init_state != InitState::INITIALIZING) {
		return;
	}

	init_timed_out = true;

	DotNetStatusIndicator *status_indicator = DotNetStatusIndicator::get_singleton();
	if (status_indicator != nullptr) {
		status_indicator->update();
	}
}
#endif

void DotNetModule::_update_initialization_state(InitState p_init_state) {
	init_state = p_init_state;

#ifdef TOOLS_ENABLED
	init_timed_out = false;

	if (p_init_state == InitState::INITIALIZING) {
		// Start a timer to check if initialization is taking too long.
		callable_mp(this, &DotNetModule::_start_init_timer).call_deferred();
	}

	DotNetStatusIndicator *status_indicator = DotNetStatusIndicator::get_singleton();
	if (status_indicator != nullptr) {
		status_indicator->update();
	}
#endif
}

#ifdef TOOLS_ENABLED
void _print_err(const String &p_error) {
	ERR_PRINT_ED(vformat(".NET: %s", p_error));
}

void DotNetModule::_set_initialization_failed(FailedState p_state, const String &p_error) {
	failed_state = p_state;
	_update_initialization_state(InitState::FAILED);

	// Print the error message deferred so the editor toaster is ready to show it,
	// otherwise it would only show in the console.
	callable_mp_static(_print_err).call_deferred(p_error);
}

void DotNetModule::_update_user_assembly_state(UserAssemblyState p_state) {
	user_assembly_state = p_state;

	DotNetStatusIndicator *status_indicator = DotNetStatusIndicator::get_singleton();
	if (status_indicator != nullptr) {
		status_indicator->update();
	}
}

void DotNetModule::_set_user_assembly_load_success(const String &p_assembly_name) {
	loaded_user_assembly_name = p_assembly_name;
	_update_user_assembly_state(UserAssemblyState::LOADED);
}

void DotNetModule::_set_user_assembly_load_failed(UserAssemblyState p_state) {
	DEV_ASSERT(p_state != UserAssemblyState::LOADED);
	loaded_user_assembly_name = "";
	_update_user_assembly_state(p_state);
}
#endif

#ifdef TOOLS_ENABLED
void DotNetModule::register_editor_settings() {
	String external_editor_hint_string = "Disabled:0";
#ifdef WINDOWS_ENABLED
	external_editor_hint_string += ",Visual Studio:1";
#endif
#ifdef MACOS_ENABLED
	external_editor_hint_string += ",Visual Studio:2";
#endif
#ifdef UNIX_ENABLED
	external_editor_hint_string += ",MonoDevelop:3";
	external_editor_hint_string += ",Visual Studio Code and VSCodium:4";
	external_editor_hint_string += ",JetBrains Rider:5";
	external_editor_hint_string += ",JetBrains Fleet:7";
	external_editor_hint_string += ",Custom:6";
#endif
	EDITOR_DEF_BASIC("dotnet/editor/external_editor", 0);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, "dotnet/editor/external_editor", PROPERTY_HINT_ENUM, external_editor_hint_string));

	EDITOR_DEF_BASIC("dotnet/editor/custom_exec_path", "");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, "dotnet/editor/custom_exec_path", PROPERTY_HINT_GLOBAL_FILE));
	EDITOR_DEF_BASIC("dotnet/editor/custom_exec_path_args", "{file}");
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::STRING, "dotnet/editor/custom_exec_path_args"));
}
#endif

void DotNetModule::register_project_settings() {
	GLOBAL_DEF("dotnet/project/assembly_name", "");

#ifdef TOOLS_ENABLED
	GLOBAL_DEF("dotnet/project/solution_directory", "");
#endif
}

#ifdef TOOLS_ENABLED
void DotNetModule::complete_initialization() {
	_update_initialization_state(InitState::INITIALIZED);
}

void DotNetModule::fail_initialization(const String &p_error) {
	_set_initialization_failed(FailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE, p_error);
}
#endif

#ifdef TOOLS_ENABLED
void DotNetModule::request_enable_dotnet_features() {
	// If the .NET module is not available, this method was called too early.
	// Wait until the editor has finished all initialization steps.
	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	// If the module is already initialized, .NET features are enabled.
	// If it's in the process of initializing, we should wait for it to succeed or fail.
	InitState init_state = module->get_init_state();
	if (init_state == InitState::INITIALIZED || init_state == InitState::INITIALIZING) {
		return;
	}

	// Otherwise, check if the editor packages are available.
	const String editor_assemblies_dir = Dirs::get_editor_assemblies_path();
	if (DirAccess::exists(editor_assemblies_dir) && FileAccess::exists(editor_assemblies_dir.path_join("Godot.EditorIntegration.dll"))) {
		// The editor packages are available, so we can just initialize the module.
		module->initialize();
		return;
	}

	// The editor packages are not available, so we need to restore them.
	// We use the welcome dialog to introduce the .NET features to the user and ask for confirmation.
	// If we're in headless mode we can't show the dialog.
	if (!DisplayServer::get_singleton()->window_can_draw()) {
		// We're in headless mode, so we can't show the dialog.
		// The user may be running code in CI. Instead of hanging forever, we exit with an error code.
		SceneTree::get_singleton()->quit(-1);
		return;
	}

	// If the welcome dialog is not available, this method was called too early.
	// Wait until the editor has finished all initialization steps.
	// DotNet::WelcomeDialog *welcome_dialog = DotNet::WelcomeDialog::get_singleton();
	// DEV_ASSERT(welcome_dialog != nullptr);
	// welcome_dialog->popup_centered();
	module->initialize();
}
#endif

#ifdef TOOLS_ENABLED
bool _project_has_csproj_files_in_dir(const String &p_root_path, bool p_include_subdirs) {
	Ref<DirAccess> dir_access = DirAccess::open(p_root_path);
	if (dir_access.is_null()) {
		return false;
	}

	for (const String &filename : dir_access->get_files()) {
		if (filename.has_extension("csproj")) {
			return true;
		}
	}

	if (p_include_subdirs) {
		for (const String &subdir : dir_access->get_directories()) {
			if (subdir == "." || subdir == "..") {
				continue;
			}
			if (_project_has_csproj_files_in_dir(p_root_path.path_join(subdir), true)) {
				return true;
			}
		}
	}

	return false;
}

bool DotNetModule::project_has_csproj_files(bool p_include_subdirs) {
	return _project_has_csproj_files_in_dir("res://", p_include_subdirs);
}
#endif

#ifdef TOOLS_ENABLED
void DotNetModule::_start_fs_watcher() {
	fs_watcher->start();
}

void DotNetModule::_on_project_assembly_changed(FileSystemWatcher::FileSystemChange change_type) {
	DEV_ASSERT(runtime_manager != nullptr);

	const String assemblies_dir = Dirs::get_project_assemblies_path();
	const String assembly_name = Dirs::get_project_assembly_name();

	switch (change_type) {
		case FileSystemWatcher::FILE_SYSTEM_CHANGE_CREATE:
			if (!runtime_manager->try_load_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to load assembly '" + assembly_name + "'.");
				_set_user_assembly_load_failed(UserAssemblyState::FAILED_TO_LOAD);
			} else {
				_set_user_assembly_load_success(assembly_name);
			}
			break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_DELETE:
			if (!runtime_manager->try_unload_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to unload assembly '" + assembly_name + "'.");
			}
			// The DLL was deleted. If the .csproj still exists the user probably cleaned
			// the build output; if it's gone too, there is no project available anymore.
			_set_user_assembly_load_failed(
					FileAccess::exists(Dirs::get_project_csproj_path())
							? UserAssemblyState::DLL_NOT_FOUND
							: UserAssemblyState::PROJECT_NOT_FOUND);
			break;

		case FileSystemWatcher::FILE_SYSTEM_CHANGE_MODIFY:
			if (!runtime_manager->try_reload_extension(assembly_name, assemblies_dir)) {
				WARN_PRINT(".NET: Failed to reload assembly '" + assembly_name + "'.");
				_set_user_assembly_load_failed(UserAssemblyState::FAILED_TO_LOAD);
			} else {
				_set_user_assembly_load_success(assembly_name);
			}
			break;
	}
}

void DotNetModule::_on_project_settings_changed() {
	if (init_state != InitState::INITIALIZED) {
		return;
	}

	const String assembly_name = Dirs::get_project_assembly_name();
	if (loaded_user_assembly_name == assembly_name) {
		// The assembly name didn't change, so no need to do anything.
		return;
	}

	change_project_assembly(assembly_name);
}

void DotNetModule::change_project_assembly(const String &p_assembly_name) {
	DEV_ASSERT(runtime_manager != nullptr);

	if (changing_project_assembly) {
		// The project assembly is already changing.
		return;
	}

	changing_project_assembly = true;

	const String old_assembly_name = loaded_user_assembly_name;
	const String old_assemblies_dir = Dirs::get_project_assemblies_path();

	// Unload the currently loaded assembly (if any).
	if (!old_assembly_name.is_empty()) {
		if (!runtime_manager->try_unload_extension(old_assembly_name, old_assemblies_dir)) {
			WARN_PRINT(".NET: Failed to unload assembly '" + old_assembly_name + "'.");
		}
	}

	// Update the project setting and persist it.
	ProjectSettings::get_singleton()->set("dotnet/project/assembly_name", p_assembly_name);
	ProjectSettings::get_singleton()->save();
	Dirs::invalidate_cached_directories();

	const String new_assembly_name = Dirs::get_project_assembly_name();
	const String new_assemblies_dir = Dirs::get_project_assemblies_path();
	DEV_ASSERT(new_assembly_name == p_assembly_name);

	// Update the file system watcher to watch the new assembly path.
	const String new_dll_path = new_assemblies_dir.path_join(new_assembly_name + ".dll");
	fs_watcher->set_path(new_dll_path);

	// Load the new assembly if it's already built, otherwise set the appropriate state.
	if (FileAccess::exists(new_dll_path)) {
		if (!runtime_manager->try_load_extension(new_assembly_name, new_assemblies_dir)) {
			WARN_PRINT(".NET: Failed to load assembly '" + new_assembly_name + "'.");
			_set_user_assembly_load_failed(UserAssemblyState::FAILED_TO_LOAD);
		} else {
			_set_user_assembly_load_success(new_assembly_name);
		}
	} else if (FileAccess::exists(Dirs::get_project_csproj_path())) {
		_set_user_assembly_load_failed(UserAssemblyState::DLL_NOT_FOUND);
	} else {
		_set_user_assembly_load_failed(UserAssemblyState::PROJECT_NOT_FOUND);
	}

	changing_project_assembly = false;
}

bool DotNetModule::try_restore_editor_packages(const String &p_editor_assemblies_path) {
	const String proj_path = p_editor_assemblies_path.path_join("RestoreEditorPackages.proj");

	// Create 'RestoreEditorPackages.proj' that restores the packages.

	Error err = DirAccess::make_dir_recursive_absolute(p_editor_assemblies_path);
	ERR_FAIL_COND_V_MSG(err != OK, false, "Could not create directory '" + p_editor_assemblies_path + "'.");
	Ref<FileAccess> proj_file = FileAccess::open(proj_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err != OK, false, "Could not open '" + proj_path + "'.");
	proj_file->store_buffer((uint8_t *)_RestoreEditorPackages_proj, strlen(_RestoreEditorPackages_proj));
	proj_file->close();

	// Build 'RestoreEditorPackages.proj' to restore the packages.

	String pipe;
	List<String> args;
	args.push_back("build");
	args.push_back(proj_path);

	int exitcode;
	err = OS::get_singleton()->execute("dotnet", args, &pipe, &exitcode, true);

	ERR_FAIL_COND_V_MSG(false, err != OK, "Failed to restore editor packages. Error: " + String(error_names[err]));
	if (exitcode != 0) {
		// Print the process output because it may contain more details about the error.
		print_line(pipe);
		ERR_FAIL_V_MSG(false, "Failed to restore editor packages. See output above for more details.");
	}

	return true;
}
#endif

#ifdef TOOLS_ENABLED
void DotNetModule::set_dotnet_sdk_info(const String &p_version, const String &p_path) {
	dotnet_sdk_version = p_version;
	dotnet_sdk_path = p_path;
}

String DotNetModule::get_dotnet_sdk_version() const {
	return dotnet_sdk_version;
}

String DotNetModule::get_dotnet_sdk_path() const {
	return dotnet_sdk_path;
}

void DotNetModule::set_editor_integration_version(const String &p_version) {
	editor_integration_version = p_version;
}

String DotNetModule::get_editor_integration_version() const {
	return editor_integration_version;
}
#endif

DotNetModule::DotNetModule() {
	singleton = this;
}

DotNetModule::~DotNetModule() {
	singleton = nullptr;

	if (runtime_manager != nullptr) {
		memdelete(runtime_manager);
	}
}

} // namespace DotNet
