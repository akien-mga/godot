/**************************************************************************/
/*  file_system_watcher.cpp                                               */
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

#include "file_system_watcher.h"

#include "core/io/file_access.h"

namespace DotNet {

void FileSystemWatcher::poll_file_system() {
	ERR_FAIL_COND_MSG(path.is_empty(), "Path to watch can't be empty.");

	if (callable.is_null()) {
		// No one is watching.
		return;
	}

	bool current_file_exists = FileAccess::exists(path);
	if (last_file_exists && !current_file_exists) {
		// File was deleted.
		callable.call((uint64_t)FILE_SYSTEM_CHANGE_DELETE);
	} else if (!last_file_exists && current_file_exists) {
		// File was created.
		callable.call((uint64_t)FILE_SYSTEM_CHANGE_CREATE);
	} else if (last_file_exists && current_file_exists) {
		uint64_t current_modified_time = FileAccess::get_modified_time(path);
		if (last_modified_time != current_modified_time) {
			// File was modified.
			callable.call((uint64_t)FILE_SYSTEM_CHANGE_MODIFY);
		}
		last_modified_time = current_modified_time;
	}
	last_file_exists = current_file_exists;

	_start_timer();
}

void FileSystemWatcher::start() {
	ERR_FAIL_COND_MSG(path.is_empty(), "Path to watch can't be empty.");

	last_file_exists = FileAccess::exists(path);
	if (last_file_exists) {
		last_modified_time = FileAccess::get_modified_time(path);
	}

	_start_timer();
}

void FileSystemWatcher::_start_timer() {
	timer = SceneTree::get_singleton()->create_timer(0.5);
	timer->connect(SNAME("timeout"), callable_mp(this, &FileSystemWatcher::poll_file_system));
}

} // namespace DotNet
