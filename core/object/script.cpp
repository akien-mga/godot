
#include "script.h"

#include "core/debugger/engine_debugger.h"
#include "core/debugger/script_debugger.h"
#include "core/io/resource_loader.h"
#include "core/variant/typed_array.h"

void Script::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			if (EngineDebugger::is_active()) {
				callable_mp(this, &Script::_set_debugger_break_language).call_deferred();
			}
		} break;
	}
}

Variant Script::_get_property_default_value(const StringName &p_property) {
	Variant ret;
	get_property_default_value(p_property, ret);
	return ret;
}

TypedArray<Dictionary> Script::_get_script_property_list() {
	TypedArray<Dictionary> ret;
	List<PropertyInfo> list;
	get_script_property_list(&list);
	for (const PropertyInfo &E : list) {
		ret.append(E.operator Dictionary());
	}
	return ret;
}

TypedArray<Dictionary> Script::_get_script_method_list() {
	TypedArray<Dictionary> ret;
	List<MethodInfo> list;
	get_script_method_list(&list);
	for (const MethodInfo &E : list) {
		ret.append(E.operator Dictionary());
	}
	return ret;
}

TypedArray<Dictionary> Script::_get_script_signal_list() {
	TypedArray<Dictionary> ret;
	List<MethodInfo> list;
	get_script_signal_list(&list);
	for (const MethodInfo &E : list) {
		ret.append(E.operator Dictionary());
	}
	return ret;
}

Dictionary Script::_get_script_constant_map() {
	Dictionary ret;
	HashMap<StringName, Variant> map;
	get_constants(&map);
	for (const KeyValue<StringName, Variant> &E : map) {
		ret[E.key] = E.value;
	}
	return ret;
}

void Script::_set_debugger_break_language() {
	if (EngineDebugger::is_active()) {
		EngineDebugger::get_script_debugger()->set_break_language(get_language());
	}
}

int Script::get_script_method_argument_count(const StringName &p_method, bool *r_is_valid) const {
	MethodInfo mi = get_method_info(p_method);

	if (mi == MethodInfo()) {
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return 0;
	}

	if (r_is_valid) {
		*r_is_valid = true;
	}
	return mi.arguments.size();
}

#ifdef TOOLS_ENABLED

PropertyInfo Script::get_class_category() const {
	String path = get_path();
	String scr_name;

	if (is_built_in()) {
		if (get_name().is_empty()) {
			scr_name = TTR("Built-in script");
		} else {
			scr_name = vformat("%s (%s)", get_name(), TTR("Built-in"));
		}
	} else {
		if (get_name().is_empty()) {
			scr_name = path.get_file();
		} else {
			scr_name = get_name();
		}
	}

	return PropertyInfo(Variant::NIL, scr_name, PROPERTY_HINT_NONE, path, PROPERTY_USAGE_CATEGORY);
}

#endif // TOOLS_ENABLED

void Script::_bind_methods() {
	ClassDB::bind_method(D_METHOD("can_instantiate"), &Script::can_instantiate);
	//ClassDB::bind_method(D_METHOD("instance_create","base_object"),&Script::instance_create);
	ClassDB::bind_method(D_METHOD("instance_has", "base_object"), &Script::instance_has);
	ClassDB::bind_method(D_METHOD("has_source_code"), &Script::has_source_code);
	ClassDB::bind_method(D_METHOD("get_source_code"), &Script::get_source_code);
	ClassDB::bind_method(D_METHOD("set_source_code", "source"), &Script::set_source_code);
	ClassDB::bind_method(D_METHOD("reload", "keep_state"), &Script::reload, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_base_script"), &Script::get_base_script);
	ClassDB::bind_method(D_METHOD("get_instance_base_type"), &Script::get_instance_base_type);

	ClassDB::bind_method(D_METHOD("get_global_name"), &Script::get_global_name);

	ClassDB::bind_method(D_METHOD("has_script_signal", "signal_name"), &Script::has_script_signal);

	ClassDB::bind_method(D_METHOD("get_script_property_list"), &Script::_get_script_property_list);
	ClassDB::bind_method(D_METHOD("get_script_method_list"), &Script::_get_script_method_list);
	ClassDB::bind_method(D_METHOD("get_script_signal_list"), &Script::_get_script_signal_list);
	ClassDB::bind_method(D_METHOD("get_script_constant_map"), &Script::_get_script_constant_map);
	ClassDB::bind_method(D_METHOD("get_property_default_value", "property"), &Script::_get_property_default_value);

	ClassDB::bind_method(D_METHOD("is_tool"), &Script::is_tool);
	ClassDB::bind_method(D_METHOD("is_abstract"), &Script::is_abstract);

	ClassDB::bind_method(D_METHOD("get_rpc_config"), &Script::_get_rpc_config_bind);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_code", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_source_code", "get_source_code");
}

void Script::reload_from_file() {
#ifdef TOOLS_ENABLED
	// Replicates how the ScriptEditor reloads script resources, which generally handles it.
	// However, when scripts are to be reloaded but aren't open in the internal editor, we go through here instead.
	const Ref<Script> rel = ResourceLoader::load(ResourceLoader::path_remap(get_path()), get_class(), ResourceFormatLoader::CACHE_MODE_IGNORE);
	if (rel.is_null()) {
		return;
	}

	set_source_code(rel->get_source_code());
	set_last_modified_time(rel->get_last_modified_time());

	// Only reload the script when there are no compilation errors to prevent printing the error messages twice.
	if (rel->is_valid()) {
		if (Engine::get_singleton()->is_editor_hint() && is_tool()) {
			get_language()->reload_tool_script(this, true);
		} else {
			// It's important to set p_keep_state to true in order to manage reloading scripts
			// that are currently instantiated.
			reload(true);
		}
	}

#else
	Resource::reload_from_file();
#endif
}
