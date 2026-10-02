/**************************************************************************/
/*  binder_common.h                                                       */
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

#include "core/typedefs.h"
#include "core/variant/method_ptrcall.h"
#include "core/variant/type_info.h"
#include "core/variant/variant.h"
#include "core/variant/variant_caster.h"
#include "core/variant/variant_internal.h"

#include <cstdio>

template <>
struct PtrToArg<char32_t> {
	_FORCE_INLINE_ static char32_t convert(const void *p_ptr) {
		return char32_t(*reinterpret_cast<const int64_t *>(p_ptr));
	}
	typedef int64_t EncodeT;
	_FORCE_INLINE_ static void encode(char32_t p_val, const void *p_ptr) {
		*(int64_t *)p_ptr = p_val;
	}
};

// Fills `r_args` with the passed arguments followed by default values for the missing ones, and returns it.
// Only called when arguments are missing. Not inlined, so this isn't repeated in every method signature's `call()`.
_NO_INLINE_ const Variant **call_get_args_with_defaults(const Variant **p_args, int p_argcount, int p_expected, const Vector<Variant> &p_default_values, const Variant **r_args);

template <typename T, typename... P, size_t... Is>
void call_with_variant_args_helper(T *p_instance, void (T::*p_method)(P...), const Variant **p_args, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	(p_instance->*p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...);
#else
	(p_instance->*p_method)(VariantCaster<P>::cast(*p_args[Is])...);
#endif // DEBUG_ENABLED
	(void)(p_args); //avoid warning
}

template <typename T, typename... P, size_t... Is>
void call_with_variant_argsc_helper(T *p_instance, void (T::*p_method)(P...) const, const Variant **p_args, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	(p_instance->*p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...);
#else
	(p_instance->*p_method)(VariantCaster<P>::cast(*p_args[Is])...);
#endif // DEBUG_ENABLED
	(void)(p_args); //avoid warning
}

template <typename T, typename... P, size_t... Is>
void call_with_ptr_args_helper(T *p_instance, void (T::*p_method)(P...), const void **p_args, IndexSequence<Is...>) {
	(p_instance->*p_method)(PtrToArg<P>::convert(p_args[Is])...);
}

template <typename T, typename... P, size_t... Is>
void call_with_ptr_argsc_helper(T *p_instance, void (T::*p_method)(P...) const, const void **p_args, IndexSequence<Is...>) {
	(p_instance->*p_method)(PtrToArg<P>::convert(p_args[Is])...);
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_ptr_args_ret_helper(T *p_instance, R (T::*p_method)(P...), const void **p_args, void *r_ret, IndexSequence<Is...>) {
	PtrToArg<R>::encode((p_instance->*p_method)(PtrToArg<P>::convert(p_args[Is])...), r_ret);
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_ptr_args_retc_helper(T *p_instance, R (T::*p_method)(P...) const, const void **p_args, void *r_ret, IndexSequence<Is...>) {
	PtrToArg<R>::encode((p_instance->*p_method)(PtrToArg<P>::convert(p_args[Is])...), r_ret);
}

template <typename T, typename... P, size_t... Is>
void call_with_ptr_args_static_helper(T *p_instance, void (*p_method)(T *, P...), const void **p_args, IndexSequence<Is...>) {
	p_method(p_instance, PtrToArg<P>::convert(p_args[Is])...);
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_ptr_args_static_retc_helper(T *p_instance, R (*p_method)(T *, P...), const void **p_args, void *r_ret, IndexSequence<Is...>) {
	PtrToArg<R>::encode(p_method(p_instance, PtrToArg<P>::convert(p_args[Is])...), r_ret);
}

template <typename R, typename... P, size_t... Is>
void call_with_ptr_args_static_method_ret_helper(R (*p_method)(P...), const void **p_args, void *r_ret, IndexSequence<Is...>) {
	PtrToArg<R>::encode(p_method(PtrToArg<P>::convert(p_args[Is])...), r_ret);
}

template <typename... P, size_t... Is>
void call_with_ptr_args_static_method_helper(void (*p_method)(P...), const void **p_args, IndexSequence<Is...>) {
	p_method(PtrToArg<P>::convert(p_args[Is])...);
}

template <typename T, typename... P, size_t... Is>
void call_with_validated_variant_args_helper(T *p_instance, void (T::*p_method)(P...), const Variant **p_args, IndexSequence<Is...>) {
	(p_instance->*p_method)((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...);
}

template <typename T, typename... P, size_t... Is>
void call_with_validated_variant_argsc_helper(T *p_instance, void (T::*p_method)(P...) const, const Variant **p_args, IndexSequence<Is...>) {
	(p_instance->*p_method)((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...);
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_validated_variant_args_ret_helper(T *p_instance, R (T::*p_method)(P...), const Variant **p_args, Variant *r_ret, IndexSequence<Is...>) {
	VariantInternalAccessor<std::decay_t<R>>::set(r_ret, (p_instance->*p_method)((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...));
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_validated_variant_args_retc_helper(T *p_instance, R (T::*p_method)(P...) const, const Variant **p_args, Variant *r_ret, IndexSequence<Is...>) {
	VariantInternalAccessor<std::decay_t<R>>::set(r_ret, (p_instance->*p_method)((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...));
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_validated_variant_args_static_retc_helper(T *p_instance, R (*p_method)(T *, P...), const Variant **p_args, Variant *r_ret, IndexSequence<Is...>) {
	VariantInternalAccessor<std::decay_t<R>>::set(r_ret, p_method(p_instance, (VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...));
}

template <typename T, typename... P, size_t... Is>
void call_with_validated_variant_args_static_helper(T *p_instance, void (*p_method)(T *, P...), const Variant **p_args, IndexSequence<Is...>) {
	p_method(p_instance, (VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...);
}

template <typename R, typename... P, size_t... Is>
void call_with_validated_variant_args_static_method_ret_helper(R (*p_method)(P...), const Variant **p_args, Variant *r_ret, IndexSequence<Is...>) {
	VariantInternalAccessor<std::decay_t<R>>::set(r_ret, p_method((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...));
}

template <typename... P, size_t... Is>
void call_with_validated_variant_args_static_method_helper(void (*p_method)(P...), const Variant **p_args, IndexSequence<Is...>) {
	p_method((VariantInternalAccessor<std::decay_t<P>>::get(p_args[Is]))...);
}

template <typename T, typename... P>
void call_with_variant_args(T *p_instance, void (T::*p_method)(P...), const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_args_helper<T, P...>(p_instance, p_method, p_args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_variant_args_dv(T *p_instance, void (T::*p_method)(P...), const Variant **p_args, int p_argcount, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_helper(p_instance, p_method, args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_variant_argsc(T *p_instance, void (T::*p_method)(P...) const, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_argsc_helper<T, P...>(p_instance, p_method, p_args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_variant_argsc_dv(T *p_instance, void (T::*p_method)(P...) const, const Variant **p_args, int p_argcount, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_argsc_helper(p_instance, p_method, args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_variant_args_ret_dv(T *p_instance, R (T::*p_method)(P...), const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_ret_helper(p_instance, p_method, args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_variant_args_retc_dv(T *p_instance, R (T::*p_method)(P...) const, const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_retc_helper(p_instance, p_method, args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_ptr_args(T *p_instance, void (T::*p_method)(P...), const void **p_args) {
	call_with_ptr_args_helper<T, P...>(p_instance, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_ptr_argsc(T *p_instance, void (T::*p_method)(P...) const, const void **p_args) {
	call_with_ptr_argsc_helper<T, P...>(p_instance, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_ptr_args_ret(T *p_instance, R (T::*p_method)(P...), const void **p_args, void *r_ret) {
	call_with_ptr_args_ret_helper<T, R, P...>(p_instance, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_ptr_args_retc(T *p_instance, R (T::*p_method)(P...) const, const void **p_args, void *r_ret) {
	call_with_ptr_args_retc_helper<T, R, P...>(p_instance, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_ptr_args_static(T *p_instance, void (*p_method)(T *, P...), const void **p_args) {
	call_with_ptr_args_static_helper<T, P...>(p_instance, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_ptr_args_static_retc(T *p_instance, R (*p_method)(T *, P...), const void **p_args, void *r_ret) {
	call_with_ptr_args_static_retc_helper<T, R, P...>(p_instance, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename R, typename... P>
void call_with_ptr_args_static_method_ret(R (*p_method)(P...), const void **p_args, void *r_ret) {
	call_with_ptr_args_static_method_ret_helper<R, P...>(p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename... P>
void call_with_ptr_args_static_method(void (*p_method)(P...), const void **p_args) {
	call_with_ptr_args_static_method_helper<P...>(p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

// Validated

template <typename T, typename... P>
void call_with_validated_variant_args(Variant *p_base, void (T::*p_method)(P...), const Variant **p_args) {
	call_with_validated_variant_args_helper<T, P...>(&VariantInternalAccessor<T>::get(p_base), p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_variant_args_ret(Variant *p_base, R (T::*p_method)(P...), const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_ret_helper<T, R, P...>(&VariantInternalAccessor<T>::get(p_base), p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_variant_args_retc(Variant *p_base, R (T::*p_method)(P...) const, const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_retc_helper<T, R, P...>(&VariantInternalAccessor<T>::get(p_base), p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_validated_variant_args_static(Variant *p_base, void (*p_method)(T *, P...), const Variant **p_args) {
	call_with_validated_variant_args_static_helper<T, P...>(&VariantInternalAccessor<T>::get(p_base), p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_variant_args_static_retc(Variant *p_base, R (*p_method)(T *, P...), const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_static_retc_helper<T, R, P...>(&VariantInternalAccessor<T>::get(p_base), p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename... P>
void call_with_validated_variant_args_static_method(void (*p_method)(P...), const Variant **p_args) {
	call_with_validated_variant_args_static_method_helper<P...>(p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename R, typename... P>
void call_with_validated_variant_args_static_method_ret(R (*p_method)(P...), const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_static_method_ret_helper<R, P...>(p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

// Validated Object

template <typename T, typename... P>
void call_with_validated_object_instance_args(T *p_base, void (T::*p_method)(P...), const Variant **p_args) {
	call_with_validated_variant_args_helper<T, P...>(p_base, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_validated_object_instance_argsc(T *p_base, void (T::*p_method)(P...) const, const Variant **p_args) {
	call_with_validated_variant_argsc_helper<T, P...>(p_base, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_object_instance_args_ret(T *p_base, R (T::*p_method)(P...), const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_ret_helper<T, R, P...>(p_base, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_object_instance_args_retc(T *p_base, R (T::*p_method)(P...) const, const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_retc_helper<T, R, P...>(p_base, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P>
void call_with_validated_object_instance_args_static(T *p_base, void (*p_method)(T *, P...), const Variant **p_args) {
	call_with_validated_variant_args_static_helper<T, P...>(p_base, p_method, p_args, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_validated_object_instance_args_static_retc(T *p_base, R (*p_method)(T *, P...), const Variant **p_args, Variant *r_ret) {
	call_with_validated_variant_args_static_retc_helper<T, R, P...>(p_base, p_method, p_args, r_ret, BuildIndexSequence<sizeof...(P)>{});
}

// GCC raises "parameter 'p_args' set but not used" when P = {},
// it's not clever enough to treat other P values as making this branch valid.
GODOT_GCC_WARNING_PUSH_AND_IGNORE("-Wunused-but-set-parameter")

// The functions below index tables built from the argument types, rather than testing each argument in turn.

template <typename... P>
Variant::Type call_get_argument_type(int p_arg) {
	if constexpr (sizeof...(P) == 0) {
		return Variant::NIL;
	} else {
		static constexpr Variant::Type types[] = { GetTypeInfo<P>::VARIANT_TYPE... };
		return (p_arg >= 0 && p_arg < (int)sizeof...(P)) ? types[p_arg] : Variant::NIL;
	}
}

template <typename... P>
PropertyInfo call_get_argument_type_info(int p_arg) {
	if constexpr (sizeof...(P) > 0) {
		static constexpr PropertyInfo (*const infos[])() = { &get_type_class_info<std::decay_t<P>>... };
		if (p_arg >= 0 && p_arg < (int)sizeof...(P)) {
			return infos[p_arg]();
		}
	}
	return PropertyInfo();
}

template <typename... P>
void call_get_argument_type_info(int p_arg, PropertyInfo &r_info) {
	if (p_arg >= 0 && p_arg < (int)sizeof...(P)) {
		r_info = call_get_argument_type_info<P...>(p_arg);
	}
}

#ifdef DEBUG_ENABLED
template <typename... P>
GodotTypeInfo::Metadata call_get_argument_metadata(int p_arg) {
	if constexpr (sizeof...(P) == 0) {
		return GodotTypeInfo::METADATA_NONE;
	} else {
		static constexpr GodotTypeInfo::Metadata metadata[] = { GetTypeInfo<P>::METADATA... };
		return (p_arg >= 0 && p_arg < (int)sizeof...(P)) ? metadata[p_arg] : GodotTypeInfo::METADATA_NONE;
	}
}
#endif // DEBUG_ENABLED

//////////////////////

template <typename T, typename R, typename... P, size_t... Is>
void call_with_variant_args_ret_helper(T *p_instance, R (T::*p_method)(P...), const Variant **p_args, Variant &r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	r_ret = VariantInternal::make((p_instance->*p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...));
#else
	r_ret = VariantInternal::make((p_instance->*p_method)(VariantCaster<P>::cast(*p_args[Is])...));
#endif
}

template <typename R, typename... P, size_t... Is>
void call_with_variant_args_static_ret(R (*p_method)(P...), const Variant **p_args, Variant &r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	r_ret = VariantInternal::make((p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...));
#else
	r_ret = VariantInternal::make((p_method)(VariantCaster<P>::cast(*p_args[Is])...));
#endif // DEBUG_ENABLED
}

template <typename... P, size_t... Is>
void call_with_variant_args_static(void (*p_method)(P...), const Variant **p_args, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	(p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...);
#else
	(p_method)(VariantCaster<P>::cast(*p_args[Is])...);
#endif // DEBUG_ENABLED
}

template <typename T, typename R, typename... P>
void call_with_variant_args_ret(T *p_instance, R (T::*p_method)(P...), const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_args_ret_helper<T, R, P...>(p_instance, p_method, p_args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_variant_args_retc_helper(T *p_instance, R (T::*p_method)(P...) const, const Variant **p_args, Variant &r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	r_ret = VariantInternal::make((p_instance->*p_method)(VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...));
#else
	r_ret = VariantInternal::make((p_instance->*p_method)(VariantCaster<P>::cast(*p_args[Is])...));
#endif // DEBUG_ENABLED
	(void)p_args;
}

template <typename R, typename... P>
void call_with_variant_args_static_ret(R (*p_method)(P...), const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_args_static_ret<R, P...>(p_method, p_args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename... P>
void call_with_variant_args_static(void (*p_method)(P...), const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_args_static<P...>(p_method, p_args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P>
void call_with_variant_args_retc(T *p_instance, R (T::*p_method)(P...) const, const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	if ((size_t)p_argcount < sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED
	call_with_variant_args_retc_helper<T, R, P...>(p_instance, p_method, p_args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename R, typename... P, size_t... Is>
void call_with_variant_args_retc_static_helper(T *p_instance, R (*p_method)(T *, P...), const Variant **p_args, Variant &r_ret, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	r_ret = VariantInternal::make((p_method)(p_instance, VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...));
#else
	r_ret = VariantInternal::make((p_method)(p_instance, VariantCaster<P>::cast(*p_args[Is])...));
#endif // DEBUG_ENABLED

	(void)p_args;
}

template <typename T, typename R, typename... P>
void call_with_variant_args_retc_static_helper_dv(T *p_instance, R (*p_method)(T *, P...), const Variant **p_args, int p_argcount, Variant &r_ret, const Vector<Variant> &p_default_values, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_retc_static_helper(p_instance, p_method, args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename T, typename... P, size_t... Is>
void call_with_variant_args_static_helper(T *p_instance, void (*p_method)(T *, P...), const Variant **p_args, Callable::CallError &r_error, IndexSequence<Is...>) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef DEBUG_ENABLED
	(p_method)(p_instance, VariantCasterAndValidate<P>::cast(p_args, Is, r_error)...);
#else
	(p_method)(p_instance, VariantCaster<P>::cast(*p_args[Is])...);
#endif // DEBUG_ENABLED

	(void)p_args;
}

template <typename T, typename... P>
void call_with_variant_args_static_helper_dv(T *p_instance, void (*p_method)(T *, P...), const Variant **p_args, int p_argcount, const Vector<Variant> &p_default_values, Callable::CallError &r_error) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_static_helper(p_instance, p_method, args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename R, typename... P>
void call_with_variant_args_static_ret_dv(R (*p_method)(P...), const Variant **p_args, int p_argcount, Variant &r_ret, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_static_ret(p_method, args, r_ret, r_error, BuildIndexSequence<sizeof...(P)>{});
}

template <typename... P>
void call_with_variant_args_static_dv(void (*p_method)(P...), const Variant **p_args, int p_argcount, Callable::CallError &r_error, const Vector<Variant> &p_default_values) {
#ifdef DEBUG_ENABLED
	if ((size_t)p_argcount > sizeof...(P)) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}

	int32_t missing = (int32_t)sizeof...(P) - (int32_t)p_argcount;
	int32_t dvs = p_default_values.size();
	if (missing > dvs) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = sizeof...(P);
		return;
	}
#endif // DEBUG_ENABLED

	const Variant *args_with_defaults[sizeof...(P) == 0 ? 1 : sizeof...(P)]; //avoid zero sized array
	const Variant **args = p_argcount >= (int)sizeof...(P) ? p_args : call_get_args_with_defaults(p_args, p_argcount, sizeof...(P), p_default_values, args_with_defaults);

	call_with_variant_args_static(p_method, args, r_error, BuildIndexSequence<sizeof...(P)>{});
}

GODOT_GCC_WARNING_POP
