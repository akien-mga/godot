/**************************************************************************/
/*  display_server_enums.h                                                */
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

namespace DisplayServerEnums {

typedef int WindowID;
typedef int IndicatorID;

enum {
	MAIN_WINDOW_ID = 0,
	INVALID_WINDOW_ID = -1,
	INVALID_INDICATOR_ID = -1
};

enum WindowMode {
	WINDOW_MODE_WINDOWED,
	WINDOW_MODE_MINIMIZED,
	WINDOW_MODE_MAXIMIZED,
	WINDOW_MODE_FULLSCREEN,
	WINDOW_MODE_EXCLUSIVE_FULLSCREEN,
};

enum WindowFlags {
	WINDOW_FLAG_RESIZE_DISABLED,
	WINDOW_FLAG_BORDERLESS,
	WINDOW_FLAG_ALWAYS_ON_TOP,
	WINDOW_FLAG_TRANSPARENT,
	WINDOW_FLAG_NO_FOCUS,
	WINDOW_FLAG_POPUP,
	WINDOW_FLAG_EXTEND_TO_TITLE,
	WINDOW_FLAG_MOUSE_PASSTHROUGH,
	WINDOW_FLAG_SHARP_CORNERS,
	WINDOW_FLAG_EXCLUDE_FROM_CAPTURE,
	WINDOW_FLAG_POPUP_WM_HINT,
	WINDOW_FLAG_MINIMIZE_DISABLED,
	WINDOW_FLAG_MAXIMIZE_DISABLED,
	WINDOW_FLAG_MAX,
};

// Separate enum otherwise we get warnings in switches not handling all values.
enum WindowFlagsBit {
	WINDOW_FLAG_RESIZE_DISABLED_BIT = (1 << WINDOW_FLAG_RESIZE_DISABLED),
	WINDOW_FLAG_BORDERLESS_BIT = (1 << WINDOW_FLAG_BORDERLESS),
	WINDOW_FLAG_ALWAYS_ON_TOP_BIT = (1 << WINDOW_FLAG_ALWAYS_ON_TOP),
	WINDOW_FLAG_TRANSPARENT_BIT = (1 << WINDOW_FLAG_TRANSPARENT),
	WINDOW_FLAG_NO_FOCUS_BIT = (1 << WINDOW_FLAG_NO_FOCUS),
	WINDOW_FLAG_POPUP_BIT = (1 << WINDOW_FLAG_POPUP),
	WINDOW_FLAG_EXTEND_TO_TITLE_BIT = (1 << WINDOW_FLAG_EXTEND_TO_TITLE),
	WINDOW_FLAG_MOUSE_PASSTHROUGH_BIT = (1 << WINDOW_FLAG_MOUSE_PASSTHROUGH),
	WINDOW_FLAG_SHARP_CORNERS_BIT = (1 << WINDOW_FLAG_SHARP_CORNERS),
	WINDOW_FLAG_EXCLUDE_FROM_CAPTURE_BIT = (1 << WINDOW_FLAG_EXCLUDE_FROM_CAPTURE),
	WINDOW_FLAG_POPUP_WM_HINT_BIT = (1 << WINDOW_FLAG_POPUP_WM_HINT),
	WINDOW_FLAG_MINIMIZE_DISABLED_BIT = (1 << WINDOW_FLAG_MINIMIZE_DISABLED),
	WINDOW_FLAG_MAXIMIZE_DISABLED_BIT = (1 << WINDOW_FLAG_MAXIMIZE_DISABLED),
};

enum WindowEvent {
	WINDOW_EVENT_MOUSE_ENTER,
	WINDOW_EVENT_MOUSE_EXIT,
	WINDOW_EVENT_FOCUS_IN,
	WINDOW_EVENT_FOCUS_OUT,
	WINDOW_EVENT_CLOSE_REQUEST,
	WINDOW_EVENT_GO_BACK_REQUEST,
	WINDOW_EVENT_DPI_CHANGE,
	WINDOW_EVENT_TITLEBAR_CHANGE,
	WINDOW_EVENT_FORCE_CLOSE,
};

enum WindowResizeEdge {
	WINDOW_EDGE_TOP_LEFT,
	WINDOW_EDGE_TOP,
	WINDOW_EDGE_TOP_RIGHT,
	WINDOW_EDGE_LEFT,
	WINDOW_EDGE_RIGHT,
	WINDOW_EDGE_BOTTOM_LEFT,
	WINDOW_EDGE_BOTTOM,
	WINDOW_EDGE_BOTTOM_RIGHT,
	WINDOW_EDGE_MAX,
};

// Keep the VSyncMode enum values in sync with the `display/window/vsync/vsync_mode`
// project setting hint.
enum VSyncMode {
	VSYNC_DISABLED,
	VSYNC_ENABLED,
	VSYNC_ADAPTIVE,
	VSYNC_MAILBOX
};

} // namespace DisplayServerEnums
