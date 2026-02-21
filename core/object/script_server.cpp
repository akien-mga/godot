#include "script_server.h"

#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/object/script_language.h"

ScriptLanguage *ScriptServer::_languages[MAX_LANGUAGES];
int ScriptServer::_language_count = 0;
bool ScriptServer::languages_ready = false;
Mutex ScriptServer::languages_mutex;
thread_local bool ScriptServer::thread_entered = false;

bool ScriptServer::scripting_enabled = true;
bool ScriptServer::reload_scripts_on_save = false;

// These need to be the last static variables in this file, since we're exploiting the reverse-order destruction of static variables.
static bool is_program_exiting = false;
struct ProgramExitGuard {
	~ProgramExitGuard() {
		is_program_exiting = true;
	}
};
static ProgramExitGuard program_exit_guard;

void ScriptServer::set_scripting_enabled(bool p_enabled) {
	scripting_enabled = p_enabled;
}

bool ScriptServer::is_scripting_enabled() {
	return scripting_enabled;
}

ScriptLanguage *ScriptServer::get_language(int p_idx) {
	MutexLock lock(languages_mutex);
	ERR_FAIL_INDEX_V(p_idx, _language_count, nullptr);
	return _languages[p_idx];
}

ScriptLanguage *ScriptServer::get_language_for_extension(const String &p_extension) {
	MutexLock lock(languages_mutex);

	for (int i = 0; i < _language_count; i++) {
		if (_languages[i] && _languages[i]->get_extension() == p_extension) {
			return _languages[i];
		}
	}

	return nullptr;
}

Error ScriptServer::register_language(ScriptLanguage *p_language) {
	MutexLock lock(languages_mutex);
	ERR_FAIL_NULL_V(p_language, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(_language_count >= MAX_LANGUAGES, ERR_UNAVAILABLE, "Script languages limit has been reach, cannot register more.");
	for (int i = 0; i < _language_count; i++) {
		const ScriptLanguage *other_language = _languages[i];
		ERR_FAIL_COND_V_MSG(other_language->get_extension() == p_language->get_extension(), ERR_ALREADY_EXISTS, vformat("A script language with extension '%s' is already registered.", p_language->get_extension()));
		ERR_FAIL_COND_V_MSG(other_language->get_name() == p_language->get_name(), ERR_ALREADY_EXISTS, vformat("A script language with name '%s' is already registered.", p_language->get_name()));
		ERR_FAIL_COND_V_MSG(other_language->get_type() == p_language->get_type(), ERR_ALREADY_EXISTS, vformat("A script language with type '%s' is already registered.", p_language->get_type()));
	}
	_languages[_language_count++] = p_language;

	// Make sure the new language is initialized in case languages have already been initialized before
	// This happens when importing the GDExtension for the first time in the editor
	if (languages_ready) {
		p_language->init();
	}

	return OK;
}

Error ScriptServer::unregister_language(const ScriptLanguage *p_language) {
	MutexLock lock(languages_mutex);

	for (int i = 0; i < _language_count; i++) {
		if (_languages[i] == p_language) {
			_language_count--;
			if (i < _language_count) {
				SWAP(_languages[i], _languages[_language_count]);
			}
			return OK;
		}
	}
	return ERR_DOES_NOT_EXIST;
}

void ScriptServer::init_languages() {
	{ // Load global classes.
		global_classes_clear();
#ifndef DISABLE_DEPRECATED
		if (ProjectSettings::get_singleton()->has_setting("_global_script_classes")) {
			Array script_classes = GLOBAL_GET("_global_script_classes");

			for (const Variant &script_class : script_classes) {
				Dictionary c = script_class;
				if (!c.has("class") || !c.has("language") || !c.has("path") || !c.has("base") || !c.has("is_abstract") || !c.has("is_tool")) {
					continue;
				}
				add_global_class(c["class"], c["base"], c["language"], c["path"], c["is_abstract"], c["is_tool"]);
			}
			ProjectSettings::get_singleton()->clear("_global_script_classes");
		}
#endif

		Array script_classes = ProjectSettings::get_singleton()->get_global_class_list();
		for (const Variant &script_class : script_classes) {
			Dictionary c = script_class;
			if (!c.has("class") || !c.has("language") || !c.has("path") || !c.has("base") || !c.has("is_abstract") || !c.has("is_tool")) {
				continue;
			}
			add_global_class(c["class"], c["base"], c["language"], c["path"], c["is_abstract"], c["is_tool"]);
		}
	}

	HashSet<ScriptLanguage *> langs_to_init;
	{
		MutexLock lock(languages_mutex);
		for (int i = 0; i < _language_count; i++) {
			if (_languages[i]) {
				langs_to_init.insert(_languages[i]);
			}
		}
	}

	for (ScriptLanguage *E : langs_to_init) {
		E->init();
	}

	{
		MutexLock lock(languages_mutex);
		languages_ready = true;
	}
}

void ScriptServer::finish_languages() {
	HashSet<ScriptLanguage *> langs_to_finish;

	{
		MutexLock lock(languages_mutex);
		for (int i = 0; i < _language_count; i++) {
			if (_languages[i]) {
				langs_to_finish.insert(_languages[i]);
			}
		}
	}

	for (ScriptLanguage *E : langs_to_finish) {
		if (CoreBind::OS::get_singleton()) {
			CoreBind::OS::get_singleton()->remove_script_loggers(E); // Unregister loggers using this script language.
		}
		E->finish();
	}

	{
		MutexLock lock(languages_mutex);
		languages_ready = false;
	}

	global_classes_clear();
}

bool ScriptServer::are_languages_initialized() {
	MutexLock lock(languages_mutex);
	return languages_ready;
}

bool ScriptServer::thread_is_entered() {
	return thread_entered;
}

void ScriptServer::set_reload_scripts_on_save(bool p_enable) {
	reload_scripts_on_save = p_enable;
}

bool ScriptServer::is_reload_scripts_on_save_enabled() {
	return reload_scripts_on_save;
}

void ScriptServer::thread_enter() {
	if (thread_entered) {
		return;
	}

	MutexLock lock(languages_mutex);
	if (!languages_ready) {
		return;
	}
	for (int i = 0; i < _language_count; i++) {
		_languages[i]->thread_enter();
	}

	thread_entered = true;
}

void ScriptServer::thread_exit() {
	if (!thread_entered) {
		return;
	}

	MutexLock lock(languages_mutex);
	if (!languages_ready) {
		return;
	}
	for (int i = 0; i < _language_count; i++) {
		_languages[i]->thread_exit();
	}

	thread_entered = false;
}

HashMap<StringName, ScriptServer::GlobalScriptClass> ScriptServer::global_classes;
HashMap<StringName, Vector<StringName>> ScriptServer::inheriters_cache;
bool ScriptServer::inheriters_cache_dirty = true;

void ScriptServer::global_classes_clear() {
	global_classes.clear();
	inheriters_cache.clear();
}

void ScriptServer::add_global_class(const StringName &p_class, const StringName &p_base, const StringName &p_language, const String &p_path, bool p_is_abstract, bool p_is_tool) {
	ERR_FAIL_COND_MSG(p_class == p_base || (global_classes.has(p_base) && get_global_class_native_base(p_base) == p_class), "Cyclic inheritance in script class.");
	GlobalScriptClass *existing = global_classes.getptr(p_class);
	if (existing) {
		// Update an existing class (only set dirty if something changed).
		if (existing->base != p_base || existing->path != p_path || existing->language != p_language) {
			existing->base = p_base;
			existing->path = p_path;
			existing->language = p_language;
			existing->is_abstract = p_is_abstract;
			existing->is_tool = p_is_tool;
			inheriters_cache_dirty = true;
		}
	} else {
		// Add new class.
		GlobalScriptClass g;
		g.language = p_language;
		g.path = p_path;
		g.base = p_base;
		g.is_abstract = p_is_abstract;
		g.is_tool = p_is_tool;
		global_classes[p_class] = g;
		inheriters_cache_dirty = true;
	}
}

void ScriptServer::remove_global_class(const StringName &p_class) {
	global_classes.erase(p_class);
	inheriters_cache_dirty = true;
}

void ScriptServer::get_inheriters_list(const StringName &p_base_type, List<StringName> *r_classes) {
	if (inheriters_cache_dirty) {
		inheriters_cache.clear();
		for (const KeyValue<StringName, GlobalScriptClass> &K : global_classes) {
			if (!inheriters_cache.has(K.value.base)) {
				inheriters_cache[K.value.base] = Vector<StringName>();
			}
			inheriters_cache[K.value.base].push_back(K.key);
		}
		for (KeyValue<StringName, Vector<StringName>> &K : inheriters_cache) {
			K.value.sort_custom<StringName::AlphCompare>();
		}
		inheriters_cache_dirty = false;
	}

	if (!inheriters_cache.has(p_base_type)) {
		return;
	}

	const Vector<StringName> &v = inheriters_cache[p_base_type];
	for (int i = 0; i < v.size(); i++) {
		r_classes->push_back(v[i]);
	}
}

void ScriptServer::get_indirect_inheriters_list(const StringName &p_base_type, List<StringName> *r_classes) {
	List<StringName> direct_inheritors;
	get_inheriters_list(p_base_type, &direct_inheritors);
	for (const StringName &inheritor : direct_inheritors) {
		r_classes->push_back(inheritor);
		get_indirect_inheriters_list(inheritor, r_classes);
	}
}

void ScriptServer::remove_global_class_by_path(const String &p_path) {
	for (const KeyValue<StringName, GlobalScriptClass> &kv : global_classes) {
		if (kv.value.path == p_path) {
			global_classes.erase(kv.key);
			inheriters_cache_dirty = true;
			return;
		}
	}
}

bool ScriptServer::is_global_class(const StringName &p_class) {
	return global_classes.has(p_class);
}

StringName ScriptServer::get_global_class_language(const StringName &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), StringName());
	return global_classes[p_class].language;
}

String ScriptServer::get_global_class_path(const String &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), String());
	return global_classes[p_class].path;
}

StringName ScriptServer::get_global_class_base(const String &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), String());
	return global_classes[p_class].base;
}

StringName ScriptServer::get_global_class_native_base(const String &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), String());
	String base = global_classes[p_class].base;
	while (global_classes.has(base)) {
		base = global_classes[base].base;
	}
	return base;
}

bool ScriptServer::is_global_class_abstract(const String &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), false);
	return global_classes[p_class].is_abstract;
}

bool ScriptServer::is_global_class_tool(const String &p_class) {
	ERR_FAIL_COND_V(!global_classes.has(p_class), false);
	return global_classes[p_class].is_tool;
}

// This function only sorts items added by this function.
// If `r_global_classes` is not empty before calling and a global sort is needed, caller must handle that separately.
void ScriptServer::get_global_class_list(LocalVector<StringName> &r_global_classes) {
	if (global_classes.is_empty()) {
		return;
	}
	r_global_classes.reserve(r_global_classes.size() + global_classes.size());
	for (const KeyValue<StringName, GlobalScriptClass> &global_class : global_classes) {
		r_global_classes.push_back(global_class.key);
	}
	SortArray<StringName, StringName::AlphCompare> sorter;
	sorter.sort(&r_global_classes[r_global_classes.size() - global_classes.size()], global_classes.size());
}

void ScriptServer::save_global_classes() {
	Dictionary class_icons;

	Array script_classes = ProjectSettings::get_singleton()->get_global_class_list();
	for (const Variant &script_class : script_classes) {
		Dictionary d = script_class;
		if (!d.has("name") || !d.has("icon")) {
			continue;
		}
		class_icons[d["name"]] = d["icon"];
	}

	LocalVector<StringName> gc;
	get_global_class_list(gc);
	Array gcarr;
	for (const StringName &class_name : gc) {
		const GlobalScriptClass &global_class = global_classes[class_name];
		Dictionary d;
		d["class"] = class_name;
		d["language"] = global_class.language;
		d["path"] = global_class.path;
		d["base"] = global_class.base;
		d["icon"] = class_icons.get(class_name, "");
		d["is_abstract"] = global_class.is_abstract;
		d["is_tool"] = global_class.is_tool;
		gcarr.push_back(d);
	}
	ProjectSettings::get_singleton()->store_global_class_list(gcarr);
}

Vector<Ref<ScriptBacktrace>> ScriptServer::capture_script_backtraces(bool p_include_variables) {
	if (is_program_exiting) {
		return Vector<Ref<ScriptBacktrace>>();
	}

	MutexLock lock(languages_mutex);
	if (!languages_ready) {
		return Vector<Ref<ScriptBacktrace>>();
	}

	Vector<Ref<ScriptBacktrace>> result;
	result.resize(_language_count);
	for (int i = 0; i < _language_count; i++) {
		result.write[i].instantiate(_languages[i], p_include_variables);
	}

	return result;
}
