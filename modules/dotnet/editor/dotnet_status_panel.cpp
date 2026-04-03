/**************************************************************************/
/*  dotnet_status_panel.cpp                                               */
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

#include "dotnet_status_panel.h"

// #include "welcome_dialog.h"

#include "../dotnet_module.h"
#include "dotnet_project_selector_dialog.h"
#include "dotnet_status_indicator.h"

#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/texture_rect.h"
#include "servers/display/display_server.h"

namespace DotNet {

void DotNetStatusPanel::_initialize_module() {
	hide();

	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);
	if (module->get_init_state() == DotNetModule::InitState::FAILED) {
		// If previously failed, retry initialization.
		module->initialize();
		return;
	}

	// WelcomeDialog *welcome_dialog = WelcomeDialog::get_singleton();
	// DEV_ASSERT(welcome_dialog != nullptr);
	// welcome_dialog->popup_centered();
	module->initialize();
}

void DotNetStatusPanel::_select_project() {
	hide();
	DotNetStatusIndicator::get_singleton()->project_selector_dialog->popup_dialog(callable_mp(this, &DotNetStatusPanel::_load_project));
}

void DotNetStatusPanel::_load_project(const String &p_path) {
	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);
	const String assembly_name = p_path.get_file().get_basename();
	module->change_project_assembly(assembly_name);
}

void DotNetStatusPanel::_open_url(const String &p_url) {
	hide();

	OS::get_singleton()->shell_open(p_url);
}

void DotNetStatusPanel::_copy_to_clipboard(const String &p_text) {
	hide();

	DisplayServer::get_singleton()->clipboard_set(p_text);
}

// TODO(@raulsntos): It may be better to reuse nodes instead of clearing and recreating them every time.
void clear_children(Node *p_node) {
	for (int i = p_node->get_child_count(); i > 0; i--) {
		Node *child = p_node->get_child(i - 1);
		p_node->remove_child(child);
		child->queue_free();
	}
}

void DotNetStatusPanel::_update_content() {
	DotNetModule *module = DotNetModule::get_singleton();
	DEV_ASSERT(module != nullptr);

	DotNetModule::InitState init_state = module->get_init_state();

	switch (init_state) {
		case DotNetModule::InitState::DISABLED: {
			status_label->set_text(TTR(".NET features disabled"));

			clear_children(details);
			if (module->project_has_csproj_files()) {
				HBoxContainer *hbox = memnew(HBoxContainer);

				TextureRect *icon = memnew(TextureRect);
				icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
				icon->set_texture(get_theme_icon(SNAME("NodeWarning"), EditorStringName(EditorIcons)));
				hbox->add_child(icon);

				Label *label = memnew(Label);
				label->set_text(TTR("Found C# files in the project."));
				hbox->add_child(label);

				details->add_child(hbox);
			} else {
				Label *label = memnew(Label);
				label->set_text(TTR("No C# files found in the project."));
				details->add_child(label);
			}

			enable_button->set_text(TTR("Enable .NET features"));
			enable_button->show();
		} break;
		case DotNetModule::InitState::INITIALIZING: {
			status_label->set_text(TTR(".NET features initializing..."));

			clear_children(details);

			if (module->is_init_timed_out()) {
				HBoxContainer *hbox = memnew(HBoxContainer);

				TextureRect *icon = memnew(TextureRect);
				icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
				icon->set_texture(get_theme_icon(SNAME("NodeWarning"), EditorStringName(EditorIcons)));
				hbox->add_child(icon);

				Label *label = memnew(Label);
				label->set_text(TTR(".NET initialization is taking longer than expected. Please check the console output for more details. If the problem persists, restart the editor."));
				hbox->add_child(label);

				details->add_child(hbox);
			}

			enable_button->hide();
		} break;
		case DotNetModule::InitState::INITIALIZED: {
			status_label->set_text(TTR(".NET features enabled"));

			// TODO(@raulsntos): Add links to documentation about using C# in Godot? Or maybe just a button to open the Welcome Dialog again?
			clear_children(details);

			// Godot.EditorIntegration line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				Label *version_label = memnew(Label);
				version_label->set_text(TTR("Godot .NET editor integration"));
				hbox->add_child(version_label);

				hbox->add_spacer();

				const String &editor_integration_version = module->get_editor_integration_version();

				LinkButton *version_info_button = memnew(LinkButton);
				version_info_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				version_info_button->set_text(editor_integration_version);
				version_info_button->set_tooltip_text(TTR("Click to copy the version information."));
				version_info_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_copy_to_clipboard).bind(vformat("Godot.EditorIntegration %s", editor_integration_version)));
				hbox->add_child(version_info_button);

				details->add_child(hbox);
			}

			// .NET SDK line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				Label *sdk_label = memnew(Label);
				sdk_label->set_text(TTR(".NET SDK"));
				hbox->add_child(sdk_label);

				hbox->add_spacer();

				const String &dotnet_sdk_version = module->get_dotnet_sdk_version();
				const String &dotnet_sdk_path = module->get_dotnet_sdk_path();

				if (dotnet_sdk_version.is_empty() || dotnet_sdk_path.is_empty()) {
					Label *sdk_info_label = memnew(Label(TTR("Not found")));
					hbox->add_child(sdk_info_label);
				} else {
					LinkButton *sdk_info_button = memnew(LinkButton);
					sdk_info_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
					sdk_info_button->set_text(dotnet_sdk_version);
					sdk_info_button->set_tooltip_text(TTR("Click to copy the version information."));
					sdk_info_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_copy_to_clipboard).bind(vformat("%s [%s]", dotnet_sdk_version, dotnet_sdk_path)));
					hbox->add_child(sdk_info_button);
				}

				details->add_child(hbox);
			}

			// Project line.
			{
				HBoxContainer *hbox = memnew(HBoxContainer);

				const String &assembly_name = module->get_loaded_user_assembly_name();

				Label *project_label = memnew(Label);
				switch (module->get_user_assembly_state()) {
					case DotNetModule::UserAssemblyState::PROJECT_NOT_FOUND: {
						// This is an acceptable scenario if there is no .csproj files at all in the workspace.
						// It indicates it's likely a project that doesn't use C# (i.e., a GDScript-only project).
						// But if there are .csproj files somewhere, it means the project settings need to be
						// configured to point to the right one.
						// IMPORTANT: We only support .csproj files in the root directory, but we still show
						// a warning if there are .csproj files in subdirectories, to let the user know that we
						// found their project may be misconfigured.
						if (module->project_has_csproj_files(true)) {
							TextureRect *icon = memnew(TextureRect);
							icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
							icon->set_texture(get_theme_icon(SNAME("NodeWarning"), EditorStringName(EditorIcons)));
							hbox->add_child(icon);
							project_label->set_text(TTR("No C# project loaded. Found C# projects in the workspace."));
						} else {
							project_label->set_text(TTR("No C# project loaded."));
						}
					} break;
					case DotNetModule::UserAssemblyState::DLL_NOT_FOUND: {
						// We found a .csproj in the expected location, but no DLL.
						// This likely means the user hasn't built the project yet, so it couldn't be loaded,
						// which is needed to register C# classes and run editor classes.
						// We let the user know about it, so they don't get confused about why their C# classes
						// don't show up in the editor, and show a build button to fix it.
						TextureRect *icon = memnew(TextureRect);
						icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
						icon->set_texture(get_theme_icon(SNAME("NodeWarning"), EditorStringName(EditorIcons)));
						hbox->add_child(icon);
						project_label->set_text(TTR("No C# project loaded. Project needs to be built."));
					} break;
					case DotNetModule::UserAssemblyState::FAILED_TO_LOAD: {
						TextureRect *icon = memnew(TextureRect);
						icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
						icon->set_texture(get_theme_icon(SNAME("StatusError"), EditorStringName(EditorIcons)));
						hbox->add_child(icon);
						project_label->set_text(TTR("Failed to load C# project."));
					} break;
					case DotNetModule::UserAssemblyState::LOADED: {
						project_label->set_text(vformat("%s.csproj", module->get_loaded_user_assembly_name()));
					} break;
				}
				hbox->add_child(project_label);

				hbox->add_spacer();

				LinkButton *select_project_button = memnew(LinkButton);
				select_project_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
				if (assembly_name.is_empty()) {
					select_project_button->set_text(TTR("Open"));
				} else {
					select_project_button->set_text(TTR("Change"));
				}
				select_project_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_select_project));
				hbox->add_child(select_project_button);

				// TODO(@raulsntos): Hiding the button because changing the loaded assembly crashes the editor. Need to investigate further, but it looks like unloading when a scene is open with C# nodes makes it crash because it still tries to access some of the registered class callbacks that are now dangling pointers.
				if (module->get_user_assembly_state() != DotNetModule::UserAssemblyState::LOADED) {
					select_project_button->show();
				} else {
					select_project_button->hide();
				}

				if (module->get_user_assembly_state() == DotNetModule::UserAssemblyState::DLL_NOT_FOUND) {
					// If the C# project needs to be built, show a build button.
					// The FileSystemWatcher should handle reloading the assembly once the DLL is generated.
					LinkButton *build_button = memnew(LinkButton);
					build_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
					build_button->set_text(TTR("Build"));
					build_button->connect(SceneStringName(pressed), callable_mp(EditorNode::get_singleton(), &EditorNode::call_build));
					hbox->add_child(build_button);
				}

				details->add_child(hbox);
			}

			enable_button->hide();
		} break;
		case DotNetModule::InitState::FAILED: {
			status_label->set_text(TTR(".NET features disabled - Failed to initialize"));

			clear_children(details);

			HBoxContainer *hbox = memnew(HBoxContainer);
			details->add_child(hbox);

			TextureRect *icon = memnew(TextureRect);
			icon->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
			icon->set_texture(get_theme_icon(SNAME("StatusError"), EditorStringName(EditorIcons)));
			hbox->add_child(icon);

			switch (module->get_failed_state()) {
				case DotNetModule::FailedState::DOTNET_SDK_NOT_FOUND: {
					Label *label = memnew(Label);
					label->set_text(TTR("The .NET SDK was not found."));
					hbox->add_child(label);

					hbox->add_spacer();

					LinkButton *download_button = memnew(LinkButton);
					download_button->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
					download_button->set_text(TTR("Download"));
					download_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_open_url).bind("https://get.dot.net/"));
					hbox->add_child(download_button);
				} break;
				case DotNetModule::FailedState::EDITOR_INTEGRATION_RESTORE_FAILED: {
					Label *label = memnew(Label);
					label->set_text(TTR("Downloading the .NET editor integration failed."));
					hbox->add_child(label);
				} break;
				case DotNetModule::FailedState::EDITOR_INTEGRATION_FAILED_TO_INITIALIZE: {
					Label *label = memnew(Label);
					label->set_text(TTR("Loading the .NET editor integration failed."));
					hbox->add_child(label);
				} break;
				default: {
					Label *label = memnew(Label);
					label->set_text(TTR("An unknown error occurred. There may be more details in the console output."));
					hbox->add_child(label);
				} break;
			}

			enable_button->set_text(TTR("Retry"));
			enable_button->show();
		} break;
	}
}

void DotNetStatusPanel::refresh() {
	_update_content();
}

DotNetStatusPanel::DotNetStatusPanel() {
	VBoxContainer *vbox = memnew(VBoxContainer);
	vbox->set_custom_minimum_size(Size2(350 * EDSCALE, 0));
	add_child(vbox);

	status_label = memnew(Label);
	status_label->set_theme_type_variation("HeaderSmall");
	vbox->add_child(status_label);

	details = memnew(VBoxContainer);
	vbox->add_child(details);

	HBoxContainer *button_container = memnew(HBoxContainer);
	button_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	vbox->add_child(button_container);

	enable_button = memnew(Button);
	enable_button->set_text(TTR("Enable .NET features"));
	enable_button->connect(SceneStringName(pressed), callable_mp(this, &DotNetStatusPanel::_initialize_module));
	button_container->add_child(enable_button);
}

} // namespace DotNet
