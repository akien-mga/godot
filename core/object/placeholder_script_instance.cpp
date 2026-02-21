#include "placeholder_script_instance.h"

#include "core/config/engine.h"
#include "core/object/script_language.h"

bool PlaceHolderScriptInstance::set(const StringName &p_name, const Variant &p_value) {
	if (script->is_placeholder_fallback_enabled()) {
		return false;
	}

	if (values.has(p_name)) {
		Variant defval;
		if (script->get_property_default_value(p_name, defval)) {
			// The evaluate function ensures that a NIL variant is equal to e.g. an empty Resource.
			// Simply doing defval == p_value does not do this.
			if (Variant::evaluate(Variant::OP_EQUAL, defval, p_value)) {
				values.erase(p_name);
				return true;
			}
		}
		values[p_name] = p_value;
		return true;
	} else {
		Variant defval;
		if (script->get_property_default_value(p_name, defval)) {
			if (Variant::evaluate(Variant::OP_NOT_EQUAL, defval, p_value)) {
				values[p_name] = p_value;
			}
			return true;
		}
	}
	return false;
}

bool PlaceHolderScriptInstance::get(const StringName &p_name, Variant &r_ret) const {
	if (values.has(p_name)) {
		r_ret = values[p_name];
		return true;
	}

	if (constants.has(p_name)) {
		r_ret = constants[p_name];
		return true;
	}

	if (!script->is_placeholder_fallback_enabled()) {
		Variant defval;
		if (script->get_property_default_value(p_name, defval)) {
			r_ret = defval;
			return true;
		}
	}

	return false;
}

void PlaceHolderScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const {
	if (script->is_placeholder_fallback_enabled()) {
		for (const PropertyInfo &E : properties) {
			p_properties->push_back(E);
		}
	} else {
		for (const PropertyInfo &E : properties) {
			PropertyInfo pinfo = E;
			p_properties->push_back(E);
		}
	}
}

Variant::Type PlaceHolderScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (values.has(p_name)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return values[p_name].get_type();
	}

	if (constants.has(p_name)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		return constants[p_name].get_type();
	}

	if (r_is_valid) {
		*r_is_valid = false;
	}

	return Variant::NIL;
}

void PlaceHolderScriptInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script->is_placeholder_fallback_enabled()) {
		return;
	}

	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool PlaceHolderScriptInstance::has_method(const StringName &p_method) const {
	if (script->is_placeholder_fallback_enabled()) {
		return false;
	}

	if (script.is_valid()) {
		Ref<Script> scr = script;
		while (scr.is_valid()) {
			if (scr->has_method(p_method)) {
				return true;
			}
			scr = scr->get_base_script();
		}
	}
	return false;
}

Variant PlaceHolderScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
#if TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint()) {
		return String("Attempt to call a method on a placeholder instance. Check if the script is in tool mode.");
	} else {
		return String("Attempt to call a method on a placeholder instance. Probably a bug, please report.");
	}
#else
	return Variant();
#endif // TOOLS_ENABLED
}

void PlaceHolderScriptInstance::update(const List<PropertyInfo> &p_properties, const HashMap<StringName, Variant> &p_values) {
	HashSet<StringName> new_values;
	for (const PropertyInfo &E : p_properties) {
		if (E.usage & (PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP | PROPERTY_USAGE_CATEGORY)) {
			continue;
		}

		StringName n = E.name;
		new_values.insert(n);

		if (!values.has(n) || (E.type != Variant::NIL && values[n].get_type() != E.type)) {
			if (p_values.has(n)) {
				values[n] = p_values[n];
			}
		}
	}

	properties = p_properties;
	List<StringName> to_remove;

	for (KeyValue<StringName, Variant> &E : values) {
		if (!new_values.has(E.key)) {
			to_remove.push_back(E.key);
		}

		Variant defval;
		if (script->get_property_default_value(E.key, defval)) {
			//remove because it's the same as the default value
			if (defval == E.value) {
				to_remove.push_back(E.key);
			}
		}
	}

	while (to_remove.size()) {
		values.erase(to_remove.front()->get());
		to_remove.pop_front();
	}

	if (owner && owner->get_script_instance() == this) {
		owner->notify_property_list_changed();
	}
	//change notify

	constants.clear();
	script->get_constants(&constants);
}

void PlaceHolderScriptInstance::property_set_fallback(const StringName &p_name, const Variant &p_value, bool *r_valid) {
	if (script->is_placeholder_fallback_enabled()) {
		HashMap<StringName, Variant>::Iterator E = values.find(p_name);

		if (E) {
			E->value = p_value;
		} else {
			values.insert(p_name, p_value);
		}

		bool found = false;
		for (const PropertyInfo &F : properties) {
			if (F.name == p_name) {
				found = true;
				break;
			}
		}
		if (!found) {
			PropertyHint hint = PROPERTY_HINT_NONE;
			const Object *obj = p_value.get_validated_object();
			if (obj && obj->is_class("Node")) {
				hint = PROPERTY_HINT_NODE_TYPE;
			}
			properties.push_back(PropertyInfo(p_value.get_type(), p_name, hint, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE));
		}
	}

	if (r_valid) {
		*r_valid = false; // Cannot change the value in either case
	}
}

Variant PlaceHolderScriptInstance::property_get_fallback(const StringName &p_name, bool *r_valid) {
	if (script->is_placeholder_fallback_enabled()) {
		HashMap<StringName, Variant>::ConstIterator E = values.find(p_name);

		if (E) {
			if (r_valid) {
				*r_valid = true;
			}
			return E->value;
		}

		E = constants.find(p_name);
		if (E) {
			if (r_valid) {
				*r_valid = true;
			}
			return E->value;
		}
	}

	if (r_valid) {
		*r_valid = false;
	}

	return Variant();
}

PlaceHolderScriptInstance::PlaceHolderScriptInstance(ScriptLanguage *p_language, Ref<Script> p_script, Object *p_owner) :
		owner(p_owner),
		language(p_language),
		script(p_script) {
}

PlaceHolderScriptInstance::~PlaceHolderScriptInstance() {
	if (script.is_valid()) {
		script->_placeholder_erased(this);
	}
}
