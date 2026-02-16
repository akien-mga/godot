/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "dotnet_module.h"

#ifdef TOOLS_ENABLED
#include "core/object/callable_mp.h"
#include "editor/dotnet_source_code_plugin.h"
#include "editor/dotnet_status_indicator.h"
#include "editor/editor_internal.h"
#include "editor/welcome_dialog.h"

#include "editor/editor_node.h"
#include "editor/extension/extension_source_code_manager.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/settings/editor_command_palette.h"
#endif

using namespace DotNet;

DotNetModule *module = nullptr;

#ifdef TOOLS_ENABLED
static void _editor_init() {
	DotNetModule::register_editor_settings();

	Ref<DotNetSourceCodePlugin> source_code_plugin;
	source_code_plugin.instantiate();
	ExtensionSourceCodeManager::get_singleton()->add_plugin(source_code_plugin);

	DotNet::WelcomeDialog *welcome_dialog = memnew(DotNet::WelcomeDialog);
	EditorNode::get_singleton()->add_child(welcome_dialog);

	DotNet::DotNetStatusIndicator *status_indicator = memnew(DotNet::DotNetStatusIndicator);
	EditorNode::get_bottom_panel()->add_status_indicator(status_indicator);

	// Set the initial status.
	status_indicator->update();

	// TODO(@raulsntos): Disabled for now since the Welcome Dialog implementation is not finished yet. We'll continue to automatically initialize without confirmation on startup for now.
	// // If the module hasn't been initialized yet and isn't in the process of initializing,
	// // but there is a .csproj file, show the welcome dialog automatically.
	// if (module->get_init_state() == DotNetModule::InitState::DISABLED) {
	// 	if (FileAccess::exists(Dirs::get_project_csproj_path())) {
	// 		EditorFileSystem *efs = EditorFileSystem::get_singleton();
	// 		if (efs != nullptr) {
	// 			efs->connect(SNAME("filesystem_changed"),
	// 					callable_mp((Window *)welcome_dialog, &Window::popup_centered).bind(Size2()),
	// 					Object::CONNECT_ONE_SHOT);
	// 		}
	// 	}
	// }

	// Command palette shortcuts.
	EditorCommandPalette *command_palette = EditorCommandPalette::get_singleton();
	if (command_palette != nullptr) {
		// TODO(@raulsntos): Use `callable_mp((Window *)welcome_dialog, &Window::popup_centered).bind(Size2())` instead once the Welcome Dialog is implemented, this ensures the Welcome Dialog is always shown regardless of whether the .NET features are enabled or not, so the command can serve as a way to reopen the Welcome Dialog.
		command_palette->add_command(TTRC("Enable .NET Features"), "dotnet/enable", callable_mp_static(&DotNetModule::request_enable_dotnet_features));
		command_palette->add_command(TTRC("Toggle .NET Status Panel"), "dotnet/toggle_status_panel", callable_mp(status_indicator, &DotNet::DotNetStatusIndicator::toggle_status_panel));
	}
}
#endif

void initialize_dotnet_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
			EditorNode::add_init_callback(_editor_init);
#endif
			DotNetModule::register_project_settings();
		} break;

		case MODULE_INITIALIZATION_LEVEL_SCENE: {
#ifdef TOOLS_ENABLED
			DotNet::EditorInternal::register_functions();
#endif

			module = memnew(DotNetModule);
			if (module->should_initialize()) {
				module->initialize();
			}
		} break;

		default: {
		}
	}
}

void uninitialize_dotnet_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		// We need to make sure the .NET module is uninitialized after every C# GDExtension,
		// so always use the last level (since they are uninitialized in reverse order, that's CORE).
		case MODULE_INITIALIZATION_LEVEL_CORE: {
			if (module != nullptr) {
				memdelete(module);
			}
		} break;

		default: {
		}
	}
}
