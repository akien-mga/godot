/**************************************************************************/
/*  dotnet_status_indicator.h                                             */
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

#include "dotnet_status_panel.h"

#include "scene/gui/box_container.h"

class Button;
class PanelContainer;
class StyleBoxFlat;

namespace DotNet {

class DotNetProjectSelectorDialog;

class DotNetStatusIndicator : public HBoxContainer {
	GDCLASS(DotNetStatusIndicator, HBoxContainer);

	friend class DotNetStatusPanel;

private:
	static DotNetStatusIndicator *singleton;

	Button *main_button = nullptr;
	DotNetStatusPanel *status_panel = nullptr;
	DotNetProjectSelectorDialog *project_selector_dialog = nullptr;

	void _update_indicator();
	void _update_panel_position();

	void _show_status_panel();
	void _hide_status_panel();

	void _draw_button();

protected:
	void _notification(int p_what);

public:
	static DotNetStatusIndicator *get_singleton();

	void update();

	void toggle_status_panel();

	DotNetStatusIndicator();
	~DotNetStatusIndicator();
};

} // namespace DotNet
