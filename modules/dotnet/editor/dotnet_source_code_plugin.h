/**************************************************************************/
/*  dotnet_source_code_plugin.h                                           */
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

#include "editor/extension/editor_extension_source_code_plugin.h"

namespace DotNet {

class DotNetSourceCodePlugin : public EditorExtensionSourceCodePlugin {
private:
	static DotNetSourceCodePlugin *singleton;

	struct ParsedTemplate {
		String name;
		String description;
		String content;
	};
	HashMap<String, ParsedTemplate> parsed_templates;

	/**
	 * Holds a reference to the 'EditorExtensionSourceCodePlugin' implementation on the
	 * C# side, provided by the 'Godot.EditorIntegration' assembly.
	 *
	 * We implement part of the plugin in C++ so it's available even before initializing
	 * the .NET module. But non-critical parts are implemented on C# where we can use
	 * Roslyn and have better reflection.
	 */
	EditorExtensionSourceCodePlugin *dotnet_source_code_plugin = nullptr;

public:
	static DotNetSourceCodePlugin *get_singleton();

	void set_dotnet_source_code_plugin(EditorExtensionSourceCodePlugin *p_source_code_plugin);
	void unset_dotnet_source_code_plugin(EditorExtensionSourceCodePlugin *p_source_code_plugin);

protected:
	virtual bool can_handle_object(const Object *p_object) const override;

	virtual String get_source_path(const StringName &p_class_name) const override;

	virtual bool overrides_external_editor() const override;
	virtual Error open_in_external_editor(const String &p_source_path, int p_line, int p_col) const override;

	virtual String get_language_name() const override;
	virtual Ref<Texture2D> get_language_icon() const override;

	virtual void configure_select_path_dialog(int p_path_index, EditorFileDialog *p_dialog) const override;
	virtual String adjust_path(int p_path_index, const String &p_class_name, const String &p_base_path, const String &p_old_path) const override;

	virtual bool is_using_templates() const override { return true; }
	virtual PackedStringArray get_available_templates(const String &p_base_class_name) const override;
	virtual String get_template_display_name(const String &p_template_id) const override;
	virtual String get_template_description(const String &p_template_id) const override;
	virtual TypedArray<Dictionary> get_template_options(const String &p_template_id) const override;

	virtual bool can_use_template_files() const override { return true; }
	virtual bool can_handle_template_file(const String &p_template_path) override;
	virtual String get_template_file_display_name(const String &p_template_path) override;
	virtual String get_template_file_description(const String &p_template_path) override;

	virtual bool can_create_class_source() const override { return true; }
	virtual Error create_class_source(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths) const override;
	virtual Error create_class_source_from_template_id(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_id, const Dictionary &p_template_options) const override;
	virtual Error create_class_source_from_template_file(const String &p_class_name, const String &p_base_class_name, const PackedStringArray &p_paths, const String &p_template_path) override;

	virtual void validate_class_name(ValidationContext *p_validation_context, const String &p_class_name) const override;
	virtual void validate_path(ValidationContext *p_validation_context, int p_path_index, const String &p_path) const override;
	virtual void validate_template_option(ValidationContext *p_validation_context, const String &p_template_id, const String &p_option_name, const Variant &p_value) const override;

	virtual bool can_edit_class_source() const override { return true; }
	virtual Error add_method_func(const StringName &p_class_name, const String &p_method_name, const PackedStringArray &p_args) const override;

public:
	DotNetSourceCodePlugin();
	~DotNetSourceCodePlugin();
};

} // namespace DotNet
