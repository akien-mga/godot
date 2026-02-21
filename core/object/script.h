
#pragma once

#include "core/io/resource.h"

#ifdef TOOLS_ENABLED
#include "core/doc_data.h"
#endif

class PlaceHolderScriptInstance;
class ScriptLanguage;

template <typename T>
class TypedArray;

class Script : public Resource {
	GDCLASS(Script, Resource);
	OBJ_SAVE_TYPE(Script);

protected:
	// Scripts are reloaded via the Script Editor when edited in Godot,
	// the LSP server when edited in a connected external editor, or
	// through EditorFileSystem::_update_script_documentation when updated directly on disk.
	virtual bool editor_can_reload_from_file() override { return false; }
	void _notification(int p_what);
	static void _bind_methods();

	friend class PlaceHolderScriptInstance;
	virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) {}

	Variant _get_property_default_value(const StringName &p_property);
	TypedArray<Dictionary> _get_script_property_list();
	TypedArray<Dictionary> _get_script_method_list();
	TypedArray<Dictionary> _get_script_signal_list();
	Dictionary _get_script_constant_map();

	void _set_debugger_break_language();

	Variant _get_rpc_config_bind() const {
		return get_rpc_config().duplicate(true);
	}

public:
	static constexpr AncestralClass static_ancestral_class = AncestralClass::SCRIPT;

	virtual void reload_from_file() override;

	virtual bool can_instantiate() const = 0;

	virtual Ref<Script> get_base_script() const = 0; //for script inheritance
	virtual StringName get_global_name() const = 0;
	virtual bool inherits_script(const Ref<Script> &p_script) const = 0;

	virtual StringName get_instance_base_type() const = 0; // this may not work in all scripts, will return empty if so
	virtual ScriptInstance *instance_create(Object *p_this) = 0;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) { return nullptr; }
	virtual bool instance_has(const Object *p_this) const = 0;

	virtual bool has_source_code() const = 0;
	virtual String get_source_code() const = 0;
	virtual void set_source_code(const String &p_code) = 0;
	virtual Error reload(bool p_keep_state = false) = 0;

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const = 0;
	virtual Vector<DocData::ClassDoc> get_documentation() const = 0;
	virtual String get_class_icon_path() const = 0;
	virtual PropertyInfo get_class_category() const;
#endif // TOOLS_ENABLED

	// TODO: In the next compat breakage rename to `*_script_*` to disambiguate from `Object::has_method()`.
	virtual bool has_method(const StringName &p_method) const = 0;
	virtual bool has_static_method(const StringName &p_method) const { return false; }

	virtual int get_script_method_argument_count(const StringName &p_method, bool *r_is_valid = nullptr) const;

	virtual MethodInfo get_method_info(const StringName &p_method) const = 0;

	virtual bool is_tool() const = 0;
	virtual bool is_valid() const = 0;
	virtual bool is_abstract() const = 0;

	virtual ScriptLanguage *get_language() const = 0;

	virtual bool has_script_signal(const StringName &p_signal) const = 0;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const = 0;

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const = 0;

	virtual void update_exports() {} //editor tool
	virtual void get_script_method_list(List<MethodInfo> *p_list) const = 0;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const = 0;

	virtual int get_member_line(const StringName &p_member) const { return -1; }

	virtual void get_constants(HashMap<StringName, Variant> *p_constants) {}
	virtual void get_members(HashSet<StringName> *p_members) {}

	virtual bool is_placeholder_fallback_enabled() const { return false; }

	virtual const Variant get_rpc_config() const = 0;

	Script() {
		_define_ancestry(AncestralClass::SCRIPT);
	}
};
