/**************************************************************************/
/*  dotnet_project_selector_dialog.cpp                                    */
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

#include "dotnet_project_selector_dialog.h"

#include "core/io/dir_access.h"
#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"

namespace DotNet {

void DotNetProjectSelectorDialog::_populate_items(const String &p_filter) {
	item_list->clear();

	Ref<DirAccess> dir = DirAccess::open("res://");
	ERR_FAIL_COND(dir.is_null());

	for (const String &filename : dir->get_files()) {
		if (!filename.ends_with(".csproj")) {
			continue;
		}

		if (!p_filter.is_empty() && filename.findn(p_filter) == -1) {
			continue;
		}

		const String path = "res://" + filename;
		int idx = item_list->add_item(filename);
		item_list->set_item_metadata(idx, path);
	}

	// Select the first item by default, if any.
	if (item_list->get_item_count() > 0) {
		item_list->select(0);
	}

	get_ok_button()->set_disabled(item_list->get_selected_items().is_empty());
}

void DotNetProjectSelectorDialog::_search_text_changed(const String &p_text) {
	_populate_items(p_text);
}

void DotNetProjectSelectorDialog::_search_box_gui_input(const Ref<InputEvent> &p_ie) {
	Ref<InputEventKey> key_event = p_ie;
	if (key_event.is_null() || !key_event->is_pressed()) {
		return;
	}

	const int item_count = item_list->get_item_count();
	if (item_count == 0) {
		return;
	}

	switch (key_event->get_keycode()) {
		case Key::UP:
		case Key::DOWN: {
			Vector<int> selected = item_list->get_selected_items();
			int current = selected.is_empty() ? -1 : selected[0];

			int next;
			if (key_event->get_keycode() == Key::UP) {
				next = (current <= 0) ? item_count - 1 : current - 1;
			} else {
				next = (current < 0 || current >= item_count - 1) ? 0 : current + 1;
			}

			item_list->select(next);
			item_list->ensure_current_is_visible();
			get_ok_button()->set_disabled(false);
			search_box->accept_event();
		} break;
		default:
			break;
	}
}

void DotNetProjectSelectorDialog::popup_dialog(const Callable &p_callback) {
	ERR_FAIL_COND(!p_callback.is_valid());

	item_selected_callback = p_callback;
	search_box->clear();
	_populate_items();
	search_box->grab_focus();

	get_ok_button()->set_disabled(item_list->get_selected_items().is_empty());

	popup_centered(Size2i(600, 400));
}

void DotNetProjectSelectorDialog::ok_pressed() {
	Vector<int> selected = item_list->get_selected_items();
	if (selected.is_empty()) {
		return;
	}
	const String path = item_list->get_item_metadata(selected[0]);
	item_selected_callback.call(path);
}

void DotNetProjectSelectorDialog::_item_activated(int p_index) {
	const String path = item_list->get_item_metadata(p_index);
	item_selected_callback.call(path);
	hide();
}

DotNetProjectSelectorDialog::DotNetProjectSelectorDialog() {
	set_title(TTR("Select C# Project"));

	VBoxContainer *vbc = memnew(VBoxContainer);
	vbc->add_theme_constant_override("separation", 0);
	add_child(vbc);

	{
		MarginContainer *mc = memnew(MarginContainer);
		mc->add_theme_constant_override("margin_top", 6);
		mc->add_theme_constant_override("margin_bottom", 6);
		mc->add_theme_constant_override("margin_left", 1);
		mc->add_theme_constant_override("margin_right", 1);
		vbc->add_child(mc);

		search_box = memnew(LineEdit);
		search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		search_box->set_placeholder(TTR("Search projects..."));
		search_box->set_clear_button_enabled(true);
		mc->add_child(search_box);
	}

	{
		item_list = memnew(ItemList);
		item_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		item_list->set_custom_minimum_size(Size2i(0, 300));
		vbc->add_child(item_list);
	}

	search_box->connect(SceneStringName(text_changed), callable_mp(this, &DotNetProjectSelectorDialog::_search_text_changed));
	search_box->connect(SceneStringName(gui_input), callable_mp(this, &DotNetProjectSelectorDialog::_search_box_gui_input));
	item_list->connect(SNAME("item_activated"), callable_mp(this, &DotNetProjectSelectorDialog::_item_activated));
	register_text_enter(search_box);

	set_min_size(Size2i(600, 400));
}

} // namespace DotNet
