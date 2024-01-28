/**************************************************************************/
/*  editor_internal.cpp                                                   */
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

#include "editor_internal.h"

#include "../utils/dirs_utils.h"
#include "../utils/macos_utils.h"

#include "core/extension/gdextension.h"
#include "editor/editor_node.h"
#include "editor/export/lipo.h"
#include "editor/run/editor_run_bar.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/box_container.h"

namespace DotNet {
namespace EditorInternal {

void _get_editor_assemblies_path(GDExtensionUninitializedStringNamePtr r_dest) {
	memnew_placement(r_dest, String(Dirs::get_editor_assemblies_path()));
}

void _get_project_assemblies_path(GDExtensionUninitializedStringNamePtr r_dest) {
	memnew_placement(r_dest, String(Dirs::get_project_assemblies_path()));
}

void _get_project_output_path(GDExtensionStringPtr p_project_path, GDExtensionUninitializedStringPtr r_dest) {
	String *project_path = (String *)p_project_path;
	memnew_placement(r_dest, String(Dirs::get_project_output_path(*project_path)));
}

void _get_project_sln_path(GDExtensionUninitializedStringPtr r_dest) {
	memnew_placement(r_dest, String(Dirs::get_project_sln_path()));
}

void _get_project_csproj_path(GDExtensionUninitializedStringPtr r_dest) {
	memnew_placement(r_dest, String(Dirs::get_project_csproj_path()));
}

void _get_project_assembly_name(GDExtensionUninitializedStringNamePtr r_dest) {
	memnew_placement(r_dest, String(Dirs::get_project_assembly_name()));
}

void _progress_add_task(GDExtensionStringPtr p_task, GDExtensionStringPtr p_label, GDExtensionInt p_steps, GDExtensionBool p_can_cancel) {
	String *task = (String *)p_task;
	String *label = (String *)p_label;
	EditorNode::get_singleton()->progress_add_task(*task, *label, p_steps, p_can_cancel);
}

GDExtensionBool _progress_task_step(GDExtensionStringPtr p_task, GDExtensionStringPtr p_state, GDExtensionInt p_step, GDExtensionBool p_force_refresh) {
	String *task = (String *)p_task;
	String *state = (String *)p_state;
	return EditorNode::get_singleton()->progress_task_step(*task, *state, p_step, p_force_refresh);
}

void _progress_end_task(GDExtensionStringPtr p_task) {
	String *task = (String *)p_task;
	EditorNode::get_singleton()->progress_end_task(*task);
}

void _show_warning(GDExtensionStringPtr p_text, GDExtensionStringPtr p_title) {
	String *text = (String *)p_text;
	String *title = (String *)p_title;
	EditorNode::get_singleton()->show_warning(*text, *title);
}

void _add_control_to_editor_run_bar(GDExtensionObjectPtr p_control) {
	Control *control = (Control *)p_control;
	EditorRunBar::get_singleton()->get_buttons_container()->add_child(control);
}

GDExtensionBool _is_macos_app_bundle_installed(GDExtensionStringPtr p_bundle_id) {
#if MACOS_ENABLED
	String *bundle_id = (String *)p_bundle_id;
	return macos_is_app_bundle_installed(*bundle_id);
#else
	return false;
#endif
}

GDExtensionBool _lipo_create_file(GDExtensionStringPtr p_output_path, void *p_files) {
	String *output_path = (String *)p_output_path;
	PackedStringArray *files = (PackedStringArray *)p_files;

	LipO lip;
	return lip.create_file(*output_path, *files);
}

// TODO: The methods below should be moved to the bindings.

void _editor_def(GDExtensionConstStringPtr p_setting, GDExtensionConstVariantPtr p_default_value, GDExtensionBool p_restart_if_changed, GDExtensionUninitializedVariantPtr r_setting) {
	String *setting = (String *)p_setting;
	Variant *default_value = (Variant *)p_default_value;
	memnew_placement(r_setting, Variant);
	Variant *ret = reinterpret_cast<Variant *>(r_setting);
	*ret = _EDITOR_DEF(*setting, *default_value, p_restart_if_changed);
}

void _editor_def_shortcut(GDExtensionConstStringPtr p_path, GDExtensionConstStringPtr p_name, GDExtensionInt p_keycode, GDExtensionBool p_physical, GDExtensionUninitializedObjectPtr r_shortcut) {
	String *path = (String *)p_path;
	String *name = (String *)p_name;
	memnew_placement(r_shortcut, GDExtensionObjectPtr(ED_SHORTCUT(*path, *name, (Key)p_keycode, p_physical).ptr()));
}

void _editor_shortcut_override(GDExtensionConstStringPtr p_path, GDExtensionConstStringPtr p_feature, GDExtensionInt p_keycode, GDExtensionBool p_physical) {
	String *path = (String *)p_path;
	String *feature = (String *)p_feature;
	ED_SHORTCUT_OVERRIDE(*path, *feature, (Key)p_keycode, p_physical);
}

#define REGISTER_INTERFACE_FUNC(m_name) GDExtension::register_interface_function(#m_name, (GDExtensionInterfaceFunctionPtr) & _##m_name)

void register_functions() {
	REGISTER_INTERFACE_FUNC(get_editor_assemblies_path);
	REGISTER_INTERFACE_FUNC(get_project_assemblies_path);
	REGISTER_INTERFACE_FUNC(get_project_output_path);
	REGISTER_INTERFACE_FUNC(get_project_sln_path);
	REGISTER_INTERFACE_FUNC(get_project_csproj_path);
	REGISTER_INTERFACE_FUNC(get_project_assembly_name);
	REGISTER_INTERFACE_FUNC(progress_add_task);
	REGISTER_INTERFACE_FUNC(progress_task_step);
	REGISTER_INTERFACE_FUNC(progress_end_task);
	REGISTER_INTERFACE_FUNC(show_warning);
	REGISTER_INTERFACE_FUNC(add_control_to_editor_run_bar);
	REGISTER_INTERFACE_FUNC(is_macos_app_bundle_installed);
	REGISTER_INTERFACE_FUNC(lipo_create_file);
	REGISTER_INTERFACE_FUNC(editor_def);
	REGISTER_INTERFACE_FUNC(editor_def_shortcut);
	REGISTER_INTERFACE_FUNC(editor_shortcut_override);
}

} // namespace EditorInternal
} // namespace DotNet
