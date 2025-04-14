/**************************************************************************/
/*  gdextension_dotnet_loader.cpp                                         */
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

#include "gdextension_dotnet_loader.h"

#include "../runtime/dotnet_runtime.h"

#include "core/extension/gdextension.h"

namespace DotNet {

Error GDExtensionDotNetLoader::open_library(const String &p_path) {
#ifdef TOOLS_ENABLED
	assembly_last_modified_time = FileAccess::get_modified_time(config->get_path());
#endif

	if (!runtime->try_open_extension(config)) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

Error GDExtensionDotNetLoader::initialize(GDExtensionInterfaceGetProcAddress p_get_proc_address, const Ref<GDExtension> &p_extension, GDExtensionInitialization *r_initialization) {
#ifdef TOOLS_ENABLED
	if (runtime->supports_reloading()) {
		// TODO(@raulsntos): Godot .NET bindings don't support reloading yet.
		// p_extension->set_reloadable(true);
	}
#endif

	GDExtensionInitializationFunction initialization_function = runtime->get_extension_entry_point(config);
	if (initialization_function == nullptr) {
		ERR_PRINT("GDExtension entry point '" + config->get_entry_point() + "' not found in library " + config->get_path());
		return ERR_CANT_RESOLVE;
	}

	GDExtensionBool ret = initialization_function(p_get_proc_address, p_extension.ptr(), r_initialization);

	if (ret) {
		return OK;
	} else {
		ERR_PRINT("GDExtension initialization function '" + config->get_entry_point() + "' returned an error.");
		return FAILED;
	}
}

void GDExtensionDotNetLoader::close_library() {
	runtime->close_extension(config);
}

bool GDExtensionDotNetLoader::has_library_changed() const {
#ifdef TOOLS_ENABLED
	// Check only that the last modified time is different (rather than checking
	// that it's newer) since some OS's (namely Windows) will preserve the modified
	// time by default when copying files.
	return FileAccess::get_modified_time(config->get_path()) != assembly_last_modified_time;
#else
	return false;
#endif
}

bool GDExtensionDotNetLoader::library_exists() const {
	return FileAccess::exists(config->get_path());
}

} // namespace DotNet
