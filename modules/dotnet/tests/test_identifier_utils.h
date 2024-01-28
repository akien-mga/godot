/**************************************************************************/
/*  test_identifier_utils.h                                               */
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

#include "tests/test_macros.h"

#include "modules/dotnet/utils/identifier_utils.h"

namespace TestDotNetIdentifierUtils {

// NOTE: These test cases are mirrored on the C# side in
// `tests/Godot.SourceGeneration.Tests/IdentifierUtilsTests.cs`
// Any change here must be reflected there, and vice versa.

TEST_CASE("[DotNet][IdentifierUtils] sanitize_name") {
	CHECK(DotNet::IdentifierUtils::sanitize_name("MyProject") == "MyProject");
	CHECK(DotNet::IdentifierUtils::sanitize_name("My.Test.Project") == "My.Test.Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("My...Project") == "My.Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("My Test Project") == "My_Test_Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("my-test-project") == "my_test_project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("3D_Game") == "_3D_Game");
	CHECK(DotNet::IdentifierUtils::sanitize_name("My.Test.3D_Game") == "My.Test._3D_Game");
	CHECK(DotNet::IdentifierUtils::sanitize_name(U"私のテストプロジェクト") == U"私のテストプロジェクト");
	CHECK(DotNet::IdentifierUtils::sanitize_name(U"１私のテストプロジェクト") == U"_１私のテストプロジェクト");
	CHECK(DotNet::IdentifierUtils::sanitize_name(U"My👍Project") == "My_Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("C++Project") == "C_Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("C++_Project") == "C_Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name("C#Project") == "C_Project");
	CHECK(DotNet::IdentifierUtils::sanitize_name(".NETProject") == "NETProject");
	CHECK(DotNet::IdentifierUtils::sanitize_name(".3DGame") == "_3DGame");
	CHECK(DotNet::IdentifierUtils::sanitize_name("---") == "UnnamedProject");
	CHECK(DotNet::IdentifierUtils::sanitize_name("") == "UnnamedProject");
	CHECK(DotNet::IdentifierUtils::sanitize_name("namespace") == "namespace");
}

} // namespace TestDotNetIdentifierUtils
