/**************************************************************************/
/*  welcome_dialog.h                                                      */
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

#include "editor/inspector/editor_inspector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/foldable_container.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"

namespace DotNet {

class WelcomeDialog : public AcceptDialog {
	GDCLASS(WelcomeDialog, AcceptDialog)

	enum ExternalEditorID {
		EXTERNAL_EDITOR_NONE,
		EXTERNAL_EDITOR_VISUAL_STUDIO,
		EXTERNAL_EDITOR_VISUAL_STUDIO_FOR_MAC,
		EXTERNAL_EDITOR_MONODEVELOP,
		EXTERNAL_EDITOR_VSCODE,
		EXTERNAL_EDITOR_JETBRAINS_RIDER,
		EXTERNAL_EDITOR_CUSTOM_EDITOR,
		EXTERNAL_EDITOR_JETBRAINS_FLEET,
	};

	class QuickOptions : public Object {
		GDCLASS(QuickOptions, Object);

	private:
		PackedStringArray properties;
		String section;

	protected:
		bool _set(const StringName &p_name, const Variant &p_value);
		bool _get(const StringName &p_name, Variant &r_ret) const;
		void _get_property_list(List<PropertyInfo> *p_list) const;
		bool _property_can_revert(const StringName &p_name) const;
		bool _property_get_revert(const StringName &p_name, Variant &r_property) const;

	public:
		void set_section(const String &p_section);
		void set_properties(PackedStringArray p_properties);
	};

	class ButtonContainer : public Button {
		GDCLASS(ButtonContainer, Button);

	private:
		Container *container = nullptr;

	protected:
		virtual Size2 get_minimum_size() const override;
		void _notification(int p_notification);

	public:
		Container *get_container() const { return container; }
		ButtonContainer(Container *p_container);
	};

private:
	static WelcomeDialog *singleton;

public:
	static WelcomeDialog *get_singleton();

private:
	bool dotnet_sdk_section_complete = false;
	TextureRect *dotnet_sdk_checkbox_icon = nullptr;
	Button *check_dotnet_sdk_button = nullptr;
	VBoxContainer *dotnet_sdk_not_found_messages_vb = nullptr;
	VBoxContainer *dotnet_sdk_found_messages_vb = nullptr;
	Label *dotnet_sdk_found_label = nullptr;
	TextureRect *dotnet_sdk_not_found_icon = nullptr;
	TextureRect *dotnet_sdk_path_hint_icon = nullptr;
	TextureRect *dotnet_sdk_found_icon = nullptr;

	bool external_editor_section_complete = false;
	FoldableContainer *external_editor_fc = nullptr;
	TextureRect *external_editor_checkbox_icon = nullptr;
	EditorInspector *external_editor_inspector = nullptr;

	bool godot_editor_integration_section_complete = false;
	TextureRect *godot_editor_integration_checkbox_icon = nullptr;
	Button *godot_editor_integration_download_button = nullptr;
	ProgressBar *godot_editor_integration_progress_bar = nullptr;
	HBoxContainer *godot_editor_integration_fail_hb = nullptr;
	HBoxContainer *godot_editor_integration_success_hb = nullptr;
	TextureRect *godot_editor_integration_fail_icon = nullptr;
	TextureRect *godot_editor_integration_success_icon = nullptr;
	Label *godot_editor_integration_fail_label = nullptr;
	LocalVector<TextureRect *> doc_icons;

	Button *cancel_button = nullptr;

	void _check_dotnet_sdk();
	void _check_godot_editor_integration();
	void _update_external_editor_settings();
	void _external_editor_settings_changed(const String &p_name);
	void _start_download_godot_editor_integration();
	static void _thread_function_download_godot_editor_integration(void *p_user_data);
	void _finish_download_godot_editor_integration(const String &p_error_message);
	void _update_status();

	WelcomeDialog::ButtonContainer *_create_button_base(Container *p_container);
	Button *_create_editor_suggestion_button(const String &p_editor_name, const String &p_url, ExternalEditorID p_external_editor_id);
	Button *_create_documentation_button(const String &p_title, const String &p_description, const String &p_url);

	void _select_editor_suggestion(const String &p_url, ExternalEditorID p_external_editor_id);
	static void _open_url(const String &p_url);
	void _on_cancel_pressed();

protected:
	virtual void ok_pressed() override;
	virtual void _pre_popup() override;
	void _notification(int p_notification);

public:
	WelcomeDialog();
	~WelcomeDialog();
};

} // namespace DotNet

VARIANT_ENUM_CAST(DotNet::WelcomeDialog::ExternalEditorID);
