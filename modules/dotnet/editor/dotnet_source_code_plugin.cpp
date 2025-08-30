/**************************************************************************/
/*  dotnet_source_code_plugin.cpp                                         */
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

#include "dotnet_source_code_plugin.h"

#include "core/extension/gdextension.h"
#include "editor/editor_node.h"
#include "scene/main/window.h"

#include "../extension/gdextension_dotnet_loader.h"
#include "../runtime/dotnet_runtime.h"

namespace DotNet {

DotNetSourceCodePlugin *DotNetSourceCodePlugin::singleton = nullptr;

DotNetSourceCodePlugin *DotNetSourceCodePlugin::get_singleton() {
	return singleton;
}

#define REQUIRES_DOTNET_EDITOR_INTEGRATION                           \
	if (dotnet_source_code_plugin == nullptr) {                      \
		ERR_PRINT_ONCE(".NET editor integration is not available."); \
		return;                                                      \
	}

#define REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ret)                    \
	if (dotnet_source_code_plugin == nullptr) {                      \
		ERR_PRINT_ONCE(".NET editor integration is not available."); \
		return ret;                                                  \
	}

void DotNetSourceCodePlugin::set_dotnet_source_code_plugin(EditorExtensionSourceCodePlugin *p_source_code_plugin) {
	ERR_FAIL_COND_MSG(dotnet_source_code_plugin != nullptr, ".NET source code plugin is already set.");
	dotnet_source_code_plugin = p_source_code_plugin;
}

void DotNetSourceCodePlugin::unset_dotnet_source_code_plugin(EditorExtensionSourceCodePlugin *p_source_code_plugin) {
	ERR_FAIL_COND_MSG(dotnet_source_code_plugin == nullptr, ".NET source code plugin is not set.");
	ERR_FAIL_COND_MSG(dotnet_source_code_plugin != p_source_code_plugin, ".NET source code plugin is not the same instance.");
	dotnet_source_code_plugin = nullptr;
}

bool DotNetSourceCodePlugin::can_handle_object(const Object *p_object) const {
	const GDExtension *library = p_object->get_extension_library();
	if (library == nullptr) {
		return false;
	}

	// If the GDExtension was loaded using the .NET loader, then the class
	// must be from a .NET assembly and we can handle it.
	Ref<GDExtensionDotNetLoader> dotnet_loader = library->get_loader();
	return dotnet_loader.is_valid();
}

String DotNetSourceCodePlugin::get_source_path(const StringName &p_class_name) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(String());
	return dotnet_source_code_plugin->get_source_path(p_class_name);
}

bool DotNetSourceCodePlugin::overrides_external_editor() const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(false);
	return dotnet_source_code_plugin->overrides_external_editor();
}

Error DotNetSourceCodePlugin::open_in_external_editor(const String &p_source_path, int p_line, int p_col) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ERR_UNAVAILABLE);
	dotnet_source_code_plugin->open_in_external_editor(p_source_path, p_line, p_col);
	return OK;
}

String DotNetSourceCodePlugin::get_language_name() const {
	return "C#";
}

Ref<Texture2D> DotNetSourceCodePlugin::get_language_icon() const {
	return EditorNode::get_singleton()->get_window()->get_editor_theme_icon("CSharpClass");
}

void DotNetSourceCodePlugin::configure_select_path_dialog(int p_path_index, EditorFileDialog *p_dialog) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION;
	dotnet_source_code_plugin->configure_select_path_dialog(p_path_index, p_dialog);
}

String DotNetSourceCodePlugin::adjust_path(int p_path_index, const String &p_class_name, const String &p_base_path, const String &p_old_path) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(p_old_path);
	return dotnet_source_code_plugin->adjust_path(p_path_index, p_class_name, p_base_path, p_old_path);
}

PackedStringArray DotNetSourceCodePlugin::get_available_templates(const String &p_base_class_name) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(PackedStringArray());
	return dotnet_source_code_plugin->get_available_templates(p_base_class_name);
}

String DotNetSourceCodePlugin::get_template_display_name(const String &p_template_id) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V("");
	return dotnet_source_code_plugin->get_template_display_name(p_template_id);
}

String DotNetSourceCodePlugin::get_template_description(const String &p_template_id) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V("");
	return dotnet_source_code_plugin->get_template_description(p_template_id);
}

TypedArray<Dictionary> DotNetSourceCodePlugin::get_template_options(const String &p_template_id) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(TypedArray<Dictionary>());
	return dotnet_source_code_plugin->get_template_options(p_template_id);
}

bool DotNetSourceCodePlugin::can_handle_template_file(const String &p_template_path) {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(false);
	return dotnet_source_code_plugin->can_handle_template_file(p_template_path);
}

String DotNetSourceCodePlugin::get_template_file_display_name(const String &p_template_path) {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V("");
	return dotnet_source_code_plugin->get_template_file_display_name(p_template_path);
}

String DotNetSourceCodePlugin::get_template_file_description(const String &p_template_path) {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V("");
	return dotnet_source_code_plugin->get_template_file_description(p_template_path);
}

Error DotNetSourceCodePlugin::create_class_source(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ERR_UNAVAILABLE);
	return dotnet_source_code_plugin->create_class_source(p_class_name, p_base_class_name, p_paths);
}

Error DotNetSourceCodePlugin::create_class_source_from_template_id(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_id, const Dictionary &p_template_options) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ERR_UNAVAILABLE);
	return dotnet_source_code_plugin->create_class_source_from_template_id(p_class_name, p_base_class_name, p_paths, p_template_id, p_template_options);
}

Error DotNetSourceCodePlugin::create_class_source_from_template_file(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_file_path) {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ERR_UNAVAILABLE);
	return dotnet_source_code_plugin->create_class_source_from_template_file(p_class_name, p_base_class_name, p_paths, p_template_file_path);
}

void DotNetSourceCodePlugin::validate_class_name(ValidationContext *p_validation_context, const String &p_class_name) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION;
	dotnet_source_code_plugin->validate_class_name(p_validation_context, p_class_name);
}

void DotNetSourceCodePlugin::validate_path(ValidationContext *p_validation_context, int p_path_index, const String &p_path) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION;
	dotnet_source_code_plugin->validate_path(p_validation_context, p_path_index, p_path);
}

void DotNetSourceCodePlugin::validate_template_option(ValidationContext *p_validation_context, const String &p_template_id, const String &p_option_name, const Variant &p_value) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION;
	dotnet_source_code_plugin->validate_template_option(p_validation_context, p_template_id, p_option_name, p_value);
}

Error DotNetSourceCodePlugin::add_method_func(const StringName &p_class_name, const String &p_method_name, const PackedStringArray &p_args) const {
	REQUIRES_DOTNET_EDITOR_INTEGRATION_V(ERR_UNAVAILABLE);
	return dotnet_source_code_plugin->add_method_func(p_class_name, p_method_name, p_args);
}

DotNetSourceCodePlugin::DotNetSourceCodePlugin() {
	singleton = this;
}

DotNetSourceCodePlugin::~DotNetSourceCodePlugin() {
	singleton = nullptr;
}

#undef REQUIRES_DOTNET_EDITOR_INTEGRATION
#undef REQUIRES_DOTNET_EDITOR_INTEGRATION_V

} // namespace DotNet
