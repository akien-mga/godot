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
private:
	static DotNetModule *singleton;
	bool initialized = false;

	static DotNetRuntimeManager *runtime_manager;

#ifdef TOOLS_ENABLED
	Ref<FileSystemWatcher> fs_watcher;
#endif

public:
	static DotNetModule *get_singleton();

	bool is_initialized() const;
	bool should_initialize();
	void initialize();

#ifdef TOOLS_ENABLED
	static void register_editor_settings();
#endif
	static void register_project_settings();

private:
#ifdef TOOLS_ENABLED
	void _start_fs_watcher();
	void on_project_assembly_changed(FileSystemWatcher::FileSystemChange change_type);

	bool try_restore_editor_packages(const String &p_editor_assemblies_path);
#endif

public:
	DotNetModule();
	~DotNetModule();
};

} // namespace DotNet
