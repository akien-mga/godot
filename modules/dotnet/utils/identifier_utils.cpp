/**************************************************************************/
/*  identifier_utils.cpp                                                  */
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

#include "identifier_utils.h"

#include "core/string/char_utils.h"

namespace DotNet {
namespace IdentifierUtils {

static bool _is_valid_identifier_part(char32_t p_char) {
	// 'is_unicode_identifier_continue' matches the 'Identifier_Part_Character' fragment in the C# specification.
	// See: https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#643-identifiers
	return p_char == '_' || is_unicode_identifier_continue(p_char);
}

static bool _is_valid_identifier_start(char32_t p_char) {
	// 'is_unicode_identifier_start' matches the 'Identifier_Start_Character' fragment in the C# specification.
	// See: https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/lexical-structure#643-identifiers
	return p_char == '_' || is_unicode_identifier_start(p_char);
}

static String _sanitize_segment(const String &p_segment) {
	const int len = p_segment.length();
	if (len == 0) {
		return String();
	}

	// Replace invalid characters with underscores and collapse consecutive underscores.
	String sanitized_segment;
	bool last_was_underscore = false;
	for (int i = 0; i < len; i++) {
		char32_t c = p_segment[i];
		if (!_is_valid_identifier_part(c)) {
			c = U'_';
		}
		if (c == U'_') {
			if (!last_was_underscore) {
				sanitized_segment += c;
			}
			last_was_underscore = true;
		} else {
			sanitized_segment += c;
			last_was_underscore = false;
		}
	}

	// Trim leading and trailing underscores.
	sanitized_segment = sanitized_segment.lstrip("_").rstrip("_");
	if (sanitized_segment.is_empty()) {
		return String();
	}

	// If the segment starts with an invalid character (e.g., a digit), prefix with an underscore.
	if (!_is_valid_identifier_start(sanitized_segment[0])) {
		sanitized_segment = "_" + sanitized_segment;
	}

	return sanitized_segment;
}

String sanitize_name(const String &p_name) {
	// The output must match `IdentifierUtils.SanitizeName` on the C# side.
	// See: `Godot.SourceGeneration/Utils/IdentifierUtils.cs`

	if (p_name.is_empty()) {
		return "UnnamedProject";
	}

	// Split on '.' to handle qualified identifiers. Each part is sanitized independently.
	Vector<String> parts = p_name.split(".");

	Vector<String> result_parts;
	for (const String &part : parts) {
		String sanitized = _sanitize_segment(part);
		if (!sanitized.is_empty()) {
			result_parts.push_back(sanitized);
		}
	}

	if (result_parts.is_empty()) {
		return "UnnamedProject";
	}

	return String(".").join(result_parts);
}

} // namespace IdentifierUtils
} // namespace DotNet
