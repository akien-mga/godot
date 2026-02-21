/**************************************************************************/
/*  script_language.h                                                     */
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

#include "core/io/resource.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/object/script.h"
#include "core/string/ustring.h"
#include "core/variant/typed_array.h"

class Resource;
class ScriptInstance;

extern uint8_t script_encryption_key[32];

class ScriptLanguage : public Object {
	GDCLASS(ScriptLanguage, Object)

protected:
	static void _bind_methods();

public:
	virtual String get_name() const = 0;

	/* LANGUAGE FUNCTIONS */
	virtual void init() = 0;
	virtual String get_type() const = 0;
	virtual String get_extension() const = 0;
	virtual void finish() = 0;

	/* EDITOR FUNCTIONS */
	struct Warning {
		int start_line = 0;
		int end_line = 0;
		int code;
		String string_code;
		String message;
	};

	struct ScriptError {
		String path;
		int line = -1;
		int column = -1;
		String message;
	};

	enum TemplateLocation {
		TEMPLATE_BUILT_IN,
		TEMPLATE_EDITOR,
		TEMPLATE_PROJECT
	};

	enum ScriptNameCasing {
		SCRIPT_NAME_CASING_AUTO,
		SCRIPT_NAME_CASING_PASCAL_CASE,
		SCRIPT_NAME_CASING_SNAKE_CASE,
		SCRIPT_NAME_CASING_KEBAB_CASE,
		SCRIPT_NAME_CASING_CAMEL_CASE,
	};

	struct ScriptTemplate {
		String inherit = "Object";
		String name;
		String description;
		String content;
		int id = 0;
		TemplateLocation origin = TemplateLocation::TEMPLATE_BUILT_IN;

		String get_hash() const {
			return itos(origin) + inherit + name;
		}
	};

	void get_core_type_words(List<String> *p_core_type_words) const;
	virtual Vector<String> get_reserved_words() const = 0;
	virtual bool is_control_flow_keyword(const String &p_string) const = 0;
	virtual Vector<String> get_comment_delimiters() const = 0;
	virtual Vector<String> get_doc_comment_delimiters() const = 0;
	virtual Vector<String> get_string_delimiters() const = 0;
	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const { return Ref<Script>(); }
	virtual Vector<ScriptTemplate> get_built_in_templates(const StringName &p_object) { return Vector<ScriptTemplate>(); }
	virtual bool is_using_templates() { return false; }
	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const = 0;
	virtual String validate_path(const String &p_path) const { return ""; }
	virtual bool supports_builtin_mode() const = 0;
	virtual bool supports_documentation() const { return false; }
	virtual bool can_inherit_from_file() const { return false; }
	virtual int find_function(const String &p_function, const String &p_code) const = 0;
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const = 0;
	virtual bool can_make_function() const { return true; }
	virtual Error open_in_external_editor(const Ref<Script> &p_script, int p_line, int p_col) { return ERR_UNAVAILABLE; }
	virtual bool overrides_external_editor() { return false; }
	virtual ScriptNameCasing preferred_file_name_casing() const { return SCRIPT_NAME_CASING_SNAKE_CASE; }

	// Keep enums in sync with:
	// scene/gui/code_edit.h - CodeEdit::CodeCompletionKind
	enum CodeCompletionKind {
		CODE_COMPLETION_KIND_CLASS,
		CODE_COMPLETION_KIND_FUNCTION,
		CODE_COMPLETION_KIND_SIGNAL,
		CODE_COMPLETION_KIND_VARIABLE,
		CODE_COMPLETION_KIND_MEMBER,
		CODE_COMPLETION_KIND_ENUM,
		CODE_COMPLETION_KIND_CONSTANT,
		CODE_COMPLETION_KIND_NODE_PATH,
		CODE_COMPLETION_KIND_FILE_PATH,
		CODE_COMPLETION_KIND_PLAIN_TEXT,
		CODE_COMPLETION_KIND_MAX
	};

	// scene/gui/code_edit.h - CodeEdit::CodeCompletionLocation
	enum CodeCompletionLocation {
		LOCATION_LOCAL = 0,
		LOCATION_PARENT_MASK = 1 << 8,
		LOCATION_OTHER_USER_CODE = 1 << 9,
		LOCATION_OTHER = 1 << 10,
	};

	struct CodeCompletionOption {
		CodeCompletionKind kind = CODE_COMPLETION_KIND_PLAIN_TEXT;
		String display;
		String insert_text;
		Color font_color;
		Ref<Resource> icon;
		Variant default_value;
		Vector<Pair<int, int>> matches;
		bool matches_dirty = true; // Must be set when mutating `matches`, so that sorting characteristics are recalculated.
		int location = LOCATION_OTHER;
		String theme_color_name;

		CodeCompletionOption() {}

		CodeCompletionOption(const String &p_text, CodeCompletionKind p_kind, int p_location = LOCATION_OTHER, const String &p_theme_color_name = "") {
			display = p_text;
			insert_text = p_text;
			kind = p_kind;
			location = p_location;
			theme_color_name = p_theme_color_name;
		}

		TypedArray<int> get_option_characteristics(const String &p_base);
		void clear_characteristics();
		TypedArray<int> get_option_cached_characteristics() const;

	private:
		TypedArray<int> charac;
	};

	virtual Error complete_code(const String &p_code, const String &p_path, Object *p_owner, List<CodeCompletionOption> *r_options, bool &r_force, String &r_call_hint) { return ERR_UNAVAILABLE; }

	enum LookupResultType {
		LOOKUP_RESULT_SCRIPT_LOCATION, // Use if none of the options below apply.
		LOOKUP_RESULT_CLASS,
		LOOKUP_RESULT_CLASS_CONSTANT,
		LOOKUP_RESULT_CLASS_PROPERTY,
		LOOKUP_RESULT_CLASS_METHOD,
		LOOKUP_RESULT_CLASS_SIGNAL,
		LOOKUP_RESULT_CLASS_ENUM,
		LOOKUP_RESULT_CLASS_TBD_GLOBALSCOPE, // Deprecated.
		LOOKUP_RESULT_CLASS_ANNOTATION,
		LOOKUP_RESULT_LOCAL_CONSTANT,
		LOOKUP_RESULT_LOCAL_VARIABLE,
		LOOKUP_RESULT_MAX,
	};

	struct LookupResult {
		LookupResultType type;

		// For `CLASS_*`.
		String class_name;
		String class_member;

		// For `LOCAL_*`.
		String description;
		bool is_deprecated = false;
		String deprecated_message;
		bool is_experimental = false;
		String experimental_message;

		// For `LOCAL_*`.
		String doc_type;
		String enumeration;
		bool is_bitfield = false;

		// For `LOCAL_*`.
		String value;

		// `SCRIPT_LOCATION` and `LOCAL_*` must have, `CLASS_*` can have.
		Ref<Script> script;
		String script_path;
		int location = -1;
	};

	virtual Error lookup_code(const String &p_code, const String &p_symbol, const String &p_path, Object *p_owner, LookupResult &r_result) { return ERR_UNAVAILABLE; }

	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const = 0;
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) = 0;
	virtual void add_named_global_constant(const StringName &p_name, const Variant &p_value) {}
	virtual void remove_named_global_constant(const StringName &p_name) {}

	/* MULTITHREAD FUNCTIONS */

	//some VMs need to be notified of thread creation/exiting to allocate a stack
	virtual void thread_enter() {}
	virtual void thread_exit() {}

	/* DEBUGGER FUNCTIONS */
	struct StackInfo {
		String file;
		String func;
		int line;
	};

	virtual String debug_get_error() const = 0;
	virtual int debug_get_stack_level_count() const = 0;
	virtual int debug_get_stack_level_line(int p_level) const = 0;
	virtual String debug_get_stack_level_function(int p_level) const = 0;
	virtual String debug_get_stack_level_source(int p_level) const = 0;
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual ScriptInstance *debug_get_stack_level_instance(int p_level) { return nullptr; }
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) = 0;
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) = 0;

	virtual Vector<StackInfo> debug_get_current_stack_info() { return Vector<StackInfo>(); }

	virtual void reload_all_scripts() = 0;
	virtual void reload_scripts(const Array &p_scripts, bool p_soft_reload) = 0;
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) = 0;
	/* LOADER FUNCTIONS */

	virtual void get_recognized_extensions(List<String> *p_extensions) const = 0;
	virtual void get_public_functions(List<MethodInfo> *p_functions) const = 0;
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const = 0;
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const = 0;

	struct ProfilingInfo {
		StringName signature;
		uint64_t call_count;
		uint64_t total_time;
		uint64_t self_time;
		uint64_t internal_time;
	};

	virtual void profiling_start() = 0;
	virtual void profiling_stop() = 0;
	virtual void profiling_set_save_native_calls(bool p_enable) = 0;

	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) = 0;
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) = 0;

	virtual void frame();

	virtual bool handles_global_class_type(const String &p_type) const { return false; }
	virtual String get_global_class_name(const String &p_path, String *r_base_type = nullptr, String *r_icon_path = nullptr, bool *r_is_abstract = nullptr, bool *r_is_tool = nullptr) const { return String(); }

	virtual ~ScriptLanguage() {}
};

VARIANT_ENUM_CAST(ScriptLanguage::ScriptNameCasing);
