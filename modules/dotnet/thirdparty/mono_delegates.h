// Adapted from monovm.h and assembly-functions.h to match coreclr_delegates.h.

// https://github.com/dotnet/runtime/blob/27a7fe5c4bbe0762c231b2a46162e60ee04f3cde/src/mono/mono/mini/monovm.h
// https://github.com/dotnet/runtime/blob/27a7fe5c4bbe0762c231b2a46162e60ee04f3cde/src/native/public/mono/metadata/details/assembly-functions.h

#ifndef __MONO_DELEGATES_H__
#define __MONO_DELEGATES_H__

#include "mono_types.h"

typedef int (*monovm_initialize_fn)(
		int propertyCount,
		const char **propertyKeys,
		const char **propertyValues);

// TODO: This symbol is not exported. For now we can use 'coreclr_create_delegate' instead.
// See: https://github.com/dotnet/runtime/pull/107778
// typedef int (*monovm_create_delegate_fn)(
// 		const char *assemblyName,
// 		const char *typeName,
// 		const char *methodName,
// 		/*out*/ void **delegate);
typedef int (*coreclr_create_delegate_fn)(
		/*unused*/ void *hostHandle,
		/*unused*/ unsigned int domainId,
		const char *entryPointAssemblyName,
		const char *entryPointTypeName,
		const char *entryPointMethodName,
		/*out*/ void **delegate);

typedef MonoAssembly *(*MonoAssemblyPreLoadFunc)(
		MonoAssemblyName *aname,
		char **assemblies_path,
		void* user_data);

typedef void (*mono_install_assembly_preload_hook_fn)(
		MonoAssemblyPreLoadFunc func,
		void *user_data);

typedef const char *(*mono_assembly_name_get_name_fn)(MonoAssemblyName *aname);

typedef const char *(*mono_assembly_name_get_culture_fn)(MonoAssemblyName *aname);

typedef MonoImage *(*mono_image_open_from_data_with_name_fn)(
		char *data,
		uint32_t data_len,
		mono_bool need_copy,
		/*out*/ MonoImageOpenStatus *status,
		mono_bool refonly,
		const char *name);

typedef MonoAssembly *(*mono_assembly_load_from_full_fn)(
		MonoImage *image,
		const char *fname,
		/*out*/ MonoImageOpenStatus *status,
		mono_bool refonly);

#endif // __MONO_DELEGATES_H__
