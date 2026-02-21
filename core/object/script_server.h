#pragma once

#include "core/object/script_backtrace.h"
#include "core/os/mutex.h"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"

class ScriptLanguage;
class String;

template <typename T>
class Ref;

class ScriptServer {
	enum {
		MAX_LANGUAGES = 16
	};

	static ScriptLanguage *_languages[MAX_LANGUAGES];
	static int _language_count;
	static bool languages_ready;
	static Mutex languages_mutex;
	static thread_local bool thread_entered;

	static bool scripting_enabled;
	static bool reload_scripts_on_save;

	struct GlobalScriptClass {
		StringName language;
		String path;
		StringName base;
		bool is_abstract = false;
		bool is_tool = false;
	};

	static HashMap<StringName, GlobalScriptClass> global_classes;
	static HashMap<StringName, Vector<StringName>> inheriters_cache;
	static bool inheriters_cache_dirty;

public:
	static void set_scripting_enabled(bool p_enabled);
	static bool is_scripting_enabled();
	_FORCE_INLINE_ static int get_language_count() { return _language_count; }
	static ScriptLanguage *get_language(int p_idx);
	static ScriptLanguage *get_language_for_extension(const String &p_extension);
	static Error register_language(ScriptLanguage *p_language);
	static Error unregister_language(const ScriptLanguage *p_language);

	static void set_reload_scripts_on_save(bool p_enable);
	static bool is_reload_scripts_on_save_enabled();

	static void thread_enter();
	static void thread_exit();

	static void global_classes_clear();
	static void add_global_class(const StringName &p_class, const StringName &p_base, const StringName &p_language, const String &p_path, bool p_is_abstract, bool p_is_tool);
	static void remove_global_class(const StringName &p_class);
	static void remove_global_class_by_path(const String &p_path);
	static bool is_global_class(const StringName &p_class);
	static StringName get_global_class_language(const StringName &p_class);
	static String get_global_class_path(const String &p_class);
	static StringName get_global_class_base(const String &p_class);
	static StringName get_global_class_native_base(const String &p_class);
	static bool is_global_class_abstract(const String &p_class);
	static bool is_global_class_tool(const String &p_class);
	static void get_global_class_list(LocalVector<StringName> &r_global_classes);
	static void get_inheriters_list(const StringName &p_base_type, List<StringName> *r_classes);
	static void get_indirect_inheriters_list(const StringName &p_base_type, List<StringName> *r_classes);
	static void save_global_classes();

	static Vector<Ref<ScriptBacktrace>> capture_script_backtraces(bool p_include_variables = false);

	static void init_languages();
	static void finish_languages();
	static bool are_languages_initialized();
	static bool thread_is_entered();
};
