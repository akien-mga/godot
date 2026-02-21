#pragma once

#include "core/object/ref_counted.h"
#include "core/object/script.h"
#include "core/object/script_instance.h"

class ScriptLanguage;

class PlaceHolderScriptInstance : public ScriptInstance {
	Object *owner = nullptr;
	List<PropertyInfo> properties;
	HashMap<StringName, Variant> values;
	HashMap<StringName, Variant> constants;
	ScriptLanguage *language = nullptr;
	Ref<Script> script;

public:
	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
	virtual void validate_property(PropertyInfo &p_property) const override {}

	virtual bool property_can_revert(const StringName &p_name) const override { return false; }
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const override { return false; }

	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;

	virtual int get_method_argument_count(const StringName &p_method, bool *r_is_valid = nullptr) const override {
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return 0;
	}

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;
	virtual void notification(int p_notification, bool p_reversed = false) override {}

	virtual Ref<Script> get_script() const override { return script; }

	virtual ScriptLanguage *get_language() override { return language; }

	Object *get_owner() override { return owner; }

	void update(const List<PropertyInfo> &p_properties, const HashMap<StringName, Variant> &p_values); //likely changed in editor

	virtual bool is_placeholder() const override { return true; }

	virtual void property_set_fallback(const StringName &p_name, const Variant &p_value, bool *r_valid = nullptr) override;
	virtual Variant property_get_fallback(const StringName &p_name, bool *r_valid = nullptr) override;

	virtual const Variant get_rpc_config() const override { return Variant(); }

	PlaceHolderScriptInstance(ScriptLanguage *p_language, Ref<Script> p_script, Object *p_owner);
	~PlaceHolderScriptInstance();
};
