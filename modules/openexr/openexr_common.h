/**************************************************************************/
/*  openexr_common.h                                                      */
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

#include "core/io/file_access.h"
#include "core/os/mutex.h"

#include <openexr.h>

struct OpenEXRContext {
	exr_context_t handle = nullptr;
	~OpenEXRContext() {
		if (handle) {
			exr_finish(&handle);
		}
	}
};

inline void openexr_error(exr_const_context_t p_context, int p_code, const char *p_message) {
	ERR_PRINT(vformat("OpenEXR: %s", p_message));
}

struct OpenEXRStream {
	Ref<FileAccess> file;
	Mutex mutex;

	explicit OpenEXRStream(const Ref<FileAccess> &p_file) : file(p_file) {}

	static int64_t read(exr_const_context_t p_context, void *p_userdata, void *p_buffer, uint64_t p_size, uint64_t p_offset, exr_stream_error_func_ptr_t p_error) {
		OpenEXRStream &stream = *static_cast<OpenEXRStream *>(p_userdata);
		MutexLock lock(stream.mutex);
		if (p_offset > stream.file->get_length()) {
			return -1;
		}
		stream.file->seek(p_offset);
		return stream.file->get_buffer(static_cast<uint8_t *>(p_buffer), p_size);
	}

	static int64_t size(exr_const_context_t p_context, void *p_userdata) {
		return static_cast<OpenEXRStream *>(p_userdata)->file->get_length();
	}
};
