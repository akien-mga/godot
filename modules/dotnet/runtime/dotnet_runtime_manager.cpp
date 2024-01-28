/**************************************************************************/
/*  dotnet_runtime_manager.cpp                                            */
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

#include "dotnet_runtime_manager.h"

#include "../extension/gdextension_dotnet_loader.h"
#include "hostfxr/hostfxr_dotnet_runtime.h"
#include "monovm/monovm_dotnet_runtime.h"
#include "nativeaot/nativeaot_dotnet_runtime.h"

#include "core/config/project_settings.h"
#include "core/extension/gdextension_manager.h"

namespace DotNet {

DotNetRuntimeManager *DotNetRuntimeManager::singleton = nullptr;

DotNetRuntimeManager *DotNetRuntimeManager::get_singleton() {
	return singleton;
}

const Ref<GDExtension> DotNetRuntimeManager::get_loaded_extension(const String &p_assembly_name, const String &p_assemblies_directory) const {
	GDExtensionManager *extension_manager = GDExtensionManager::get_singleton();

	for (const String &extension_path : extension_manager->get_loaded_extensions()) {
		Ref<GDExtension> extension = extension_manager->get_extension(extension_path);
		if (extension.is_null()) {
			continue;
		}

		// We only care about the extensions that were loaded with the .NET loader.
		const Ref<GDExtensionDotNetLoader> loader = extension->get_loader();
		if (loader.is_null()) {
			continue;
		}

		const Ref<GDExtensionDotNetConfig> config = loader->get_config();
		if (config.is_null()) {
			continue;
		}

		// Filter the extensions by their path.

		const String extension_basename = config->get_path().get_basename();

		const String file_name = extension_basename.get_file();
		if (file_name != p_assembly_name) {
			// The name of the file doesn't match the name of the assembly.
			continue;
		}
		if (!p_assemblies_directory.is_empty()) {
			// A directory was specified, so filter by the containing directory too.
			const String dir = extension_basename.get_base_dir();
			if (dir.simplify_path() != p_assemblies_directory.simplify_path()) {
				// The containing directory doesn't match the assemblies directory.
				continue;
			}
		}

		return extension;
	}

	return nullptr;
}

bool DotNetRuntimeManager::is_assembly_loaded(const String &p_assembly_name, const String &p_assemblies_directory) const {
	return get_loaded_extension(p_assembly_name, p_assemblies_directory).is_valid();
}

bool DotNetRuntimeManager::try_load_extension(const String &p_assembly_name, const String &p_assemblies_directory) {
	print_verbose(".NET: Loading extension with assembly name '" + p_assembly_name + "' from '" + p_assemblies_directory + "'.");

	GDExtensionManager *extension_manager = GDExtensionManager::get_singleton();

	String assemblies_dir = ProjectSettings::get_singleton()->globalize_path(p_assemblies_directory);

	// Try to load the extension with each available .NET runtime hosting API until we find one that succeeds.
	for (const Ref<DotNetRuntime> &runtime : runtimes) {
		print_verbose(".NET: Trying to load extension '" + p_assembly_name + "' with " + runtime->get_name() + ".");
		if (!runtime->try_initialize()) {
			continue;
		}

		Ref<GDExtensionDotNetConfig> config;
		if (!runtime->try_get_extension_config(p_assembly_name, assemblies_dir, config)) {
			continue;
		}

		const Ref<GDExtensionDotNetLoader> loader = memnew(GDExtensionDotNetLoader(runtime, config));
		GDExtensionManager::LoadStatus status = extension_manager->load_extension_with_loader(config->get_path(), loader);
		if (status != GDExtensionManager::LOAD_STATUS_OK && status != GDExtensionManager::LOAD_STATUS_ALREADY_LOADED) {
			continue;
		}

		print_verbose(".NET: Extension '" + p_assembly_name + "' loaded successfully with runtime " + runtime->get_name() + ".");
		return true;
	}

	print_verbose(".NET: Failed to load extension '" + p_assembly_name + "' with any available runtimes.");
	return false;
}

bool DotNetRuntimeManager::try_reload_extension(const String &p_assembly_name, const String &p_assemblies_directory) {
	print_verbose(".NET: Reloading extension with assembly name '" + p_assembly_name + "' from '" + p_assemblies_directory + "'.");

#ifndef TOOLS_ENABLED
	ERR_FAIL_V_MSG(false, "GDExtensions can only be reloaded in an editor build.");
#else
	GDExtensionManager *extension_manager = GDExtensionManager::get_singleton();

	Ref<GDExtension> extension = get_loaded_extension(p_assembly_name, p_assemblies_directory);
	if (unlikely(extension.is_null())) {
		print_verbose(".NET: Extension '" + p_assembly_name + "' is not loaded so it can't be reloaded.");
		return false;
	}

	ERR_FAIL_COND_V_MSG(unlikely(!extension->is_reloadable()), false, ".NET: Extension '" + p_assembly_name + "' is not marked as reloadable.");

	Ref<GDExtensionDotNetLoader> loader = extension->get_loader();
	Ref<GDExtensionDotNetConfig> config = loader->get_config();
	const String extension_path = config->get_path();

	GDExtensionManager::LoadStatus status = extension_manager->reload_extension(extension_path);
	if (status == GDExtensionManager::LOAD_STATUS_OK) {
		print_verbose(".NET: Extension '" + p_assembly_name + "' reloaded successfully.");
		return true;
	}

	print_verbose(".NET: Failed to reload extension '" + p_assembly_name + "'.");
	return false;
#endif
}

bool DotNetRuntimeManager::try_unload_extension(const String &p_assembly_name, const String &p_assemblies_directory) {
	print_verbose(".NET: Unloading extension with assembly name '" + p_assembly_name + "' from '" + p_assemblies_directory + "'.");

	GDExtensionManager *extension_manager = GDExtensionManager::get_singleton();

	Ref<GDExtension> extension = get_loaded_extension(p_assembly_name, p_assemblies_directory);
	if (unlikely(extension.is_null())) {
		print_verbose(".NET: Extension '" + p_assembly_name + "' is not loaded so it can't be unloaded.");
		return false;
	}

	Ref<GDExtensionDotNetLoader> loader = extension->get_loader();
	Ref<GDExtensionDotNetConfig> config = loader->get_config();
	const String extension_path = config->get_path();

	GDExtensionManager::LoadStatus status = extension_manager->unload_extension(extension_path);
	if (status == GDExtensionManager::LOAD_STATUS_OK) {
		print_verbose(".NET: Extension '" + p_assembly_name + "' unloaded successfully.");
		return true;
	}

	print_verbose(".NET: Failed to unload extension '" + p_assembly_name + "'.");
	return false;
}

DotNetRuntimeManager::DotNetRuntimeManager() {
	CRASH_COND(singleton != nullptr);
	singleton = this;

	runtimes = LocalVector<Ref<DotNetRuntime>>({
#ifdef DOTNET_HOSTFXR_ENABLED
			memnew(HostFxrDotNetRuntime),
#endif
#ifdef DOTNET_MONOVM_ENABLED
			memnew(MonoVMDotNetRuntime),
#endif
#ifdef DOTNET_NATIVEAOT_ENABLED
			memnew(NativeAOTDotNetRuntime),
#endif
	});

	// Print the list of .NET runtimes enabled in the verbose output since it can be useful for debugging.
	if (OS::get_singleton()->is_stdout_verbose()) {
		String runtime_names = "[";
		for (uint32_t i = 0; i < runtimes.size(); i++) {
			runtime_names += runtimes[i]->get_name();
			if (i < runtimes.size() - 1) {
				runtime_names += ", ";
			}
		}
		runtime_names += "]";

		print_line(".NET runtimes available: " + runtime_names);
	}
}

DotNetRuntimeManager::~DotNetRuntimeManager() {
	singleton = nullptr;

	// We need to make sure all .NET extensions are unloaded before destroying the runtimes
	// because the runtimes handle the loading/unloading of .NET assemblies and can't be done
	// without them.
	GDExtensionManager *extension_manager = GDExtensionManager::get_singleton();
	for (const String &extension_path : extension_manager->get_loaded_extensions()) {
		Ref<GDExtension> extension = extension_manager->get_extension(extension_path);
		if (extension.is_null()) {
			continue;
		}

		// We only care about the extensions that were loaded with the .NET loader.
		const Ref<GDExtensionDotNetLoader> loader = extension->get_loader();
		if (loader.is_null()) {
			continue;
		}

		GDExtensionManager::LoadStatus status = extension_manager->unload_extension(extension_path);
		if (status != GDExtensionManager::LOAD_STATUS_OK) {
			ERR_PRINT(".NET: Failed to unload extension at '" + extension_path + "'.");
		}
	}
}

} // namespace DotNet
