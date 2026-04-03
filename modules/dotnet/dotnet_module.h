/**************************************************************************/
/*  dotnet_module.h                                                       */
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

#include "runtime/dotnet_runtime_manager.h"
#include "utils/file_system_watcher.h"

#include "core/object/object.h"

namespace DotNet {

class DotNetModule : public Object {
public:
	enum class InitState {
		DISABLED,
		INITIALIZING,
		INITIALIZED,
		FAILED,
	};

	enum class FailedState {
		NONE,
		DOTNET_SDK_NOT_FOUND,
		EDITOR_INTEGRATION_RESTORE_FAILED,
		EDITOR_INTEGRATION_FAILED_TO_INITIALIZE,
	};

#ifdef TOOLS_ENABLED
	// Assembly sub-state, only meaningful when init_state == INITIALIZED.
	enum class UserAssemblyState {
		PROJECT_NOT_FOUND, // No .csproj found in the expected location.
		DLL_NOT_FOUND, // .csproj found but no DLL exists yet (project not built).
		FAILED_TO_LOAD, // DLL exists but failed to load.
		LOADED, // DLL loaded successfully.
	};
#endif

private:
	static DotNetModule *singleton;
	InitState init_state = InitState::DISABLED;
	FailedState failed_state = FailedState::NONE;

	bool init_timed_out = false;

	static DotNetRuntimeManager *runtime_manager;

#ifdef TOOLS_ENABLED
	Ref<FileSystemWatcher> fs_watcher;

	String dotnet_sdk_version;
	String dotnet_sdk_path;
	String editor_integration_version;
	String loaded_user_assembly_name;
	UserAssemblyState user_assembly_state = UserAssemblyState::PROJECT_NOT_FOUND;
	bool changing_project_assembly = false;
#endif

public:
	static DotNetModule *get_singleton();

	InitState get_init_state() const { return init_state; }
	FailedState get_failed_state() const { return failed_state; }
	bool should_initialize();
	void initialize();
#ifdef TOOLS_ENABLED
	bool is_init_timed_out() const { return init_state == InitState::INITIALIZING && init_timed_out; }
	void _start_init_timer();
	void _on_init_timeout();
#endif

private:
	void _update_initialization_state(InitState p_init_state);
#ifdef TOOLS_ENABLED
	void _set_initialization_failed(FailedState p_state, const String &p_error);
	void _update_user_assembly_state(UserAssemblyState p_state);
	void _set_user_assembly_load_success(const String &p_assembly_name);
	void _set_user_assembly_load_failed(UserAssemblyState p_state);
#endif

public:
#ifdef TOOLS_ENABLED
	static void register_editor_settings();
#endif
	static void register_project_settings();

#ifdef TOOLS_ENABLED
	/* Godot.EditorIntegration callbacks */
	void complete_initialization();
	void fail_initialization(const String &p_error);
#endif

#ifdef TOOLS_ENABLED
	static void request_enable_dotnet_features();

	static bool project_has_csproj_files(bool p_include_subdirs = false);
#endif

#ifdef TOOLS_ENABLED
private:
	void _start_fs_watcher();
	void _on_project_assembly_changed(FileSystemWatcher::FileSystemChange change_type);
	void _on_project_settings_changed();

public:
	void change_project_assembly(const String &p_assembly_name);

	static bool try_restore_editor_packages(const String &p_editor_assemblies_path);
#endif

#ifdef TOOLS_ENABLED
public:
	void set_dotnet_sdk_info(const String &p_version, const String &p_path);
	String get_dotnet_sdk_version() const;
	String get_dotnet_sdk_path() const;

	void set_editor_integration_version(const String &p_version);
	String get_editor_integration_version() const;

	String get_loaded_user_assembly_name() const { return loaded_user_assembly_name; }
	UserAssemblyState get_user_assembly_state() const { return user_assembly_state; }
#endif

public:
	DotNetModule();
	~DotNetModule();
};

} // namespace DotNet
