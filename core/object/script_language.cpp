/**************************************************************************/
/*  script_language.cpp                                                   */
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

#include "script_language.h"

void ScriptLanguage::get_core_type_words(List<String> *p_core_type_words) const {
	p_core_type_words->push_back("String");
	p_core_type_words->push_back("Vector2");
	p_core_type_words->push_back("Vector2i");
	p_core_type_words->push_back("Rect2");
	p_core_type_words->push_back("Rect2i");
	p_core_type_words->push_back("Vector3");
	p_core_type_words->push_back("Vector3i");
	p_core_type_words->push_back("Transform2D");
	p_core_type_words->push_back("Vector4");
	p_core_type_words->push_back("Vector4i");
	p_core_type_words->push_back("Plane");
	p_core_type_words->push_back("Quaternion");
	p_core_type_words->push_back("AABB");
	p_core_type_words->push_back("Basis");
	p_core_type_words->push_back("Transform3D");
	p_core_type_words->push_back("Projection");
	p_core_type_words->push_back("Color");
	p_core_type_words->push_back("StringName");
	p_core_type_words->push_back("NodePath");
	p_core_type_words->push_back("RID");
	p_core_type_words->push_back("Callable");
	p_core_type_words->push_back("Signal");
	p_core_type_words->push_back("Dictionary");
	p_core_type_words->push_back("Array");
	p_core_type_words->push_back("PackedByteArray");
	p_core_type_words->push_back("PackedInt32Array");
	p_core_type_words->push_back("PackedInt64Array");
	p_core_type_words->push_back("PackedFloat32Array");
	p_core_type_words->push_back("PackedFloat64Array");
	p_core_type_words->push_back("PackedStringArray");
	p_core_type_words->push_back("PackedVector2Array");
	p_core_type_words->push_back("PackedVector3Array");
	p_core_type_words->push_back("PackedColorArray");
	p_core_type_words->push_back("PackedVector4Array");
}

void ScriptLanguage::frame() {
}

TypedArray<int> ScriptLanguage::CodeCompletionOption::get_option_characteristics(const String &p_base) {
	// Return characteristics of the match found by order of importance.
	// Matches will be ranked by a lexicographical order on the vector returned by this function.
	// The lower values indicate better matches and that they should go before in the order of appearance.
	if (!matches_dirty) {
		return charac;
	}
	charac.clear();
	// Ensure base is not empty and at the same time that matches is not empty too.
	if (p_base.length() == 0) {
		matches_dirty = false;
		charac.push_back(location);
		return charac;
	}
	charac.push_back(matches.size());
	charac.push_back((matches[0].first == 0) ? 0 : 1);
	const char32_t *target_char = &p_base[0];
	int bad_case = 0;
	for (const Pair<int, int> &match_segment : matches) {
		const char32_t *string_to_complete_char = &display[match_segment.first];
		for (int j = 0; j < match_segment.second; j++, string_to_complete_char++, target_char++) {
			if (*string_to_complete_char != *target_char) {
				bad_case++;
			}
		}
	}
	charac.push_back(bad_case);
	charac.push_back(location);
	charac.push_back(matches[0].first);
	matches_dirty = false;
	return charac;
}

void ScriptLanguage::CodeCompletionOption::clear_characteristics() {
	charac = TypedArray<int>();
}

TypedArray<int> ScriptLanguage::CodeCompletionOption::get_option_cached_characteristics() const {
	// Only returns the cached value and warns if it was not updated since the last change of matches.
	if (matches_dirty) {
		WARN_PRINT("Characteristics are not up to date.");
	}

	return charac;
}

void ScriptLanguage::_bind_methods() {
	BIND_ENUM_CONSTANT(SCRIPT_NAME_CASING_AUTO);
	BIND_ENUM_CONSTANT(SCRIPT_NAME_CASING_PASCAL_CASE);
	BIND_ENUM_CONSTANT(SCRIPT_NAME_CASING_SNAKE_CASE);
	BIND_ENUM_CONSTANT(SCRIPT_NAME_CASING_KEBAB_CASE);
	BIND_ENUM_CONSTANT(SCRIPT_NAME_CASING_CAMEL_CASE);
}
