// Copyright 2023 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

/// Documents multiple unsafe blocks with a single safety comment.
///
/// Invoked as:
///
/// ```rust,ignore
/// safety_comment! {
///     // Non-doc comments come first.
///     /// SAFETY:
///     /// Safety comment starts on its own line.
///     macro_1!(args);
///     macro_2! { args };
///     /// SAFETY:
///     /// Subsequent safety comments are allowed but not required.
///     macro_3! { args };
/// }
/// ```
///
/// The macro invocations are emitted, each decorated with the following
/// attribute: `#[allow(clippy::undocumented_unsafe_blocks)]`.
macro_rules! safety_comment {
    (#[doc = r" SAFETY:"] $($(#[$attr:meta])* $macro:ident!$args:tt;)*) => {
        #[allow(clippy::undocumented_unsafe_blocks, unused_attributes)]
        const _: () = { $($(#[$attr])* $macro!$args;)* };
    }
}

/// Unsafely implements trait(s) for a type.
///
/// # Safety
///
/// The trait impl must be sound.
///
/// When implementing `TryFromBytes`:
/// - If no `is_bit_valid` impl is provided, then it must be valid for
///   `is_bit_valid` to unconditionally return `true`. In other words, it must
///   be the case that any initialized sequence of bytes constitutes a valid
///   instance of `$ty`.
/// - If an `is_bit_valid` impl is provided, then the impl of `is_bit_valid`
///   must only return `true` if its argument refers to a valid `$ty`.
macro_rules! unsafe_impl {
    // Implement `$trait` for `$ty` with no bounds.
    ($(#[$attr:meta])* $ty:ty: $trait:ident $(; |$candidate:ident| $is_bit_valid:expr)?) => {
        $(#[$attr])*
        unsafe impl $trait for $ty {
            unsafe_impl!(@method $trait $(; |$candidate| $is_bit_valid)?);
        }
    };

    // Implement all `$traits` for `$ty` with no bounds.
    //
    // The 2 arms under this one are there so we can apply
    // N attributes for each one of M trait implementations.
    // The simple solution of:
    //
    // ($(#[$attrs:meta])* $ty:ty: $($traits:ident),*) => {
    //     $( unsafe_impl!( $(#[$attrs])* $ty: $traits ) );*
    // }
    //
    // Won't work. The macro processor sees that the outer repetition
    // contains both $attrs and $traits and expects them to match the same
    // amount of fragments.
    //
    // To solve this we must:
    // 1. Pack the attributes into a single token tree fragment we can match over.
    // 2. Expand the traits.
    // 3. Unpack and expand the attributes.
    ($(#[$attrs:meta])* $ty:ty: $($traits:ident),*) => {
        unsafe_impl!(@impl_traits_with_packed_attrs { $(#[$attrs])* } $ty: $($traits),*)
    };

    (@impl_traits_with_packed_attrs $attrs:tt $ty:ty: $($traits:ident),*) => {
        $( unsafe_impl!(@unpack_attrs $attrs $ty: $traits); )*
    };

    (@unpack_attrs { $(#[$attrs:meta])* } $ty:ty: $traits:ident) => {
        unsafe_impl!($(#[$attrs])* $ty: $traits);
    };

    // This arm is identical to the following one, except it contains a
    // preceding `const`. If we attempt to handle these with a single arm, there
    // is an inherent ambiguity between `const` (the keyword) and `const` (the
    // ident match for `$tyvar:ident`).
    //
    // To explain how this works, consider the following invocation:
    //
    //   unsafe_impl!(const N: usize, T: ?Sized + Copy => Clone for Foo<T>);
    //
    // In this invocation, here are the assignments to meta-variables:
    //
    //   |---------------|------------|
    //   | Meta-variable | Assignment |
    //   |---------------|------------|
    //   | $constname    |  N         |
    //   | $constty      |  usize     |
    //   | $tyvar        |  T         |
    //   | $optbound     |  Sized     |
    //   | $bound        |  Copy      |
    //   | $trait        |  Clone     |
    //   | $ty           |  Foo<T>    |
    //   |---------------|------------|
    //
    // The following arm has the same behavior with the exception of the lack of
    // support for a leading `const` parameter.
    (
        $(#[$attr:meta])*
        const $constname:ident : $constty:ident $(,)?
        $($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?),*
        => $trait:ident for $ty:ty $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        unsafe_impl!(
            @inner
            $(#[$attr])*
            @const $constname: $constty,
            $($tyvar $(: $(? $optbound +)* + $($bound +)*)?,)*
            => $trait for $ty $(; |$candidate| $is_bit_valid)?
        );
    };
    (
        $(#[$attr:meta])*
        $($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?),*
        => $trait:ident for $ty:ty $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        unsafe_impl!(
            @inner
            $(#[$attr])*
            $($tyvar $(: $(? $optbound +)* + $($bound +)*)?,)*
            => $trait for $ty $(; |$candidate| $is_bit_valid)?
        );
    };
    (
        @inner
        $(#[$attr:meta])*
        $(@const $constname:ident : $constty:ident,)*
        $($tyvar:ident $(: $(? $optbound:ident +)* + $($bound:ident +)* )?,)*
        => $trait:ident for $ty:ty $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        $(#[$attr])*
        #[allow(non_local_definitions)]
        unsafe impl<$($tyvar $(: $(? $optbound +)* $($bound +)*)?),* $(, const $constname: $constty,)*> $trait for $ty {
            unsafe_impl!(@method $trait $(; |$candidate| $is_bit_valid)?);
        }
    };

    (@method TryFromBytes ; |$candidate:ident| $is_bit_valid:expr) => {
        #[allow(clippy::missing_inline_in_public_items, dead_code)]
        #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
        fn only_derive_is_allowed_to_implement_this_trait() {}

        #[inline]
        fn is_bit_valid<AA: crate::pointer::invariant::Reference>($candidate: Maybe<'_, Self, AA>) -> bool {
            $is_bit_valid
        }
    };
    (@method TryFromBytes) => {
        #[allow(clippy::missing_inline_in_public_items)]
        #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
        fn only_derive_is_allowed_to_implement_this_trait() {}
        #[inline(always)] fn is_bit_valid<AA: crate::pointer::invariant::Reference>(_: Maybe<'_, Self, AA>) -> bool { true }
    };
    (@method $trait:ident) => {
        #[allow(clippy::missing_inline_in_public_items, dead_code)]
        #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
        fn only_derive_is_allowed_to_implement_this_trait() {}
    };
    (@method $trait:ident; |$_candidate:ident| $_is_bit_valid:expr) => {
        compile_error!("Can't provide `is_bit_valid` impl for trait other than `TryFromBytes`");
    };
}

/// Implements `$trait` for `$ty` where `$ty: TransmuteFrom<$repr>` (and
/// vice-versa).
///
/// Calling this macro is safe; the internals of the macro emit appropriate
/// trait bounds which ensure that the given impl is sound.
macro_rules! impl_for_transmute_from {
    (
        $(#[$attr:meta])*
        $($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?)?
        => $trait:ident for $ty:ty [$($unsafe_cell:ident)? <$repr:ty>]
    ) => {
        $(#[$attr])*
        #[allow(non_local_definitions)]

        // SAFETY: `is_trait<T, R>` (defined and used below) requires `T:
        // TransmuteFrom<R>`, `R: TransmuteFrom<T>`, and `R: $trait`. It is
        // called using `$ty` and `$repr`, ensuring that `$ty` and `$repr` have
        // equivalent bit validity, and ensuring that `$repr: $trait`. The
        // supported traits - `TryFromBytes`, `FromZeros`, `FromBytes`, and
        // `IntoBytes` - are defined only in terms of the bit validity of a
        // type. Therefore, `$repr: $trait` ensures that `$ty: $trait` is sound.
        unsafe impl<$($tyvar $(: $(? $optbound +)* $($bound +)*)?)?> $trait for $ty {
            #[allow(dead_code, clippy::missing_inline_in_public_items)]
            #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
            fn only_derive_is_allowed_to_implement_this_trait() {
                use crate::pointer::{*, invariant::Valid};

                impl_for_transmute_from!(@assert_is_supported_trait $trait);

                fn is_trait<T, R>()
                where
                    T: TransmuteFrom<R, Valid, Valid> + ?Sized,
                    R: TransmuteFrom<T, Valid, Valid> + ?Sized,
                    R: $trait,
                {
                }

                #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
                fn f<$($tyvar $(: $(? $optbound +)* $($bound +)*)?)?>() {
                    is_trait::<$ty, $repr>();
                }
            }

            impl_for_transmute_from!(
                @is_bit_valid
                $(<$tyvar $(: $(? $optbound +)* $($bound +)*)?>)?
                $trait for $ty [$($unsafe_cell)? <$repr>]
            );
        }
    };
    (@assert_is_supported_trait TryFromBytes) => {};
    (@assert_is_supported_trait FromZeros) => {};
    (@assert_is_supported_trait FromBytes) => {};
    (@assert_is_supported_trait IntoBytes) => {};
    (
        @is_bit_valid
        $(<$tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?>)?
        TryFromBytes for $ty:ty [UnsafeCell<$repr:ty>]
    ) => {
        #[inline]
        fn is_bit_valid<A: crate::pointer::invariant::Reference>(candidate: Maybe<'_, Self, A>) -> bool {
            let c: Maybe<'_, Self, crate::pointer::invariant::Exclusive> = candidate.into_exclusive_or_pme();
            let c: Maybe<'_, $repr, _> = c.transmute::<_, _, (_, (_, (BecauseExclusive, BecauseExclusive)))>();
            // SAFETY: This macro ensures that `$repr` and `Self` have the same
            // size and bit validity. Thus, a bit-valid instance of `$repr` is
            // also a bit-valid instance of `Self`.
            <$repr as TryFromBytes>::is_bit_valid(c)
        }
    };
    (
        @is_bit_valid
        $(<$tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?>)?
        TryFromBytes for $ty:ty [<$repr:ty>]
    ) => {
        #[inline]
        fn is_bit_valid<A: crate::pointer::invariant::Reference>(candidate: Maybe<'_, Self, A>) -> bool {
            // SAFETY: This macro ensures that `$repr` and `Self` have the same
            // size and bit validity. Thus, a bit-valid instance of `$repr` is
            // also a bit-valid instance of `Self`.
            <$repr as TryFromBytes>::is_bit_valid(candidate.transmute())
        }
    };
    (
        @is_bit_valid
        $(<$tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?>)?
        $trait:ident for $ty:ty [$($unsafe_cell:ident)? <$repr:ty>]
    ) => {
        // Trait other than `TryFromBytes`; no `is_bit_valid` impl.
    };
}

/// Implements a trait for a type, bounding on each memeber of the power set of
/// a set of type variables. This is useful for implementing traits for tuples
/// or `fn` types.
///
/// The last argument is the name of a macro which will be called in every
/// `impl` block, and is expected to expand to the name of the type for which to
/// implement the trait.
///
/// For example, the invocation:
/// ```ignore
/// unsafe_impl_for_power_set!(A, B => Foo for type!(...))
/// ```
/// ...expands to:
/// ```ignore
/// unsafe impl       Foo for type!()     { ... }
/// unsafe impl<B>    Foo for type!(B)    { ... }
/// unsafe impl<A, B> Foo for type!(A, B) { ... }
/// ```
macro_rules! unsafe_impl_for_power_set {
    (
        $first:ident $(, $rest:ident)* $(-> $ret:ident)? => $trait:ident for $macro:ident!(...)
        $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        unsafe_impl_for_power_set!(
            $($rest),* $(-> $ret)? => $trait for $macro!(...)
            $(; |$candidate| $is_bit_valid)?
        );
        unsafe_impl_for_power_set!(
            @impl $first $(, $rest)* $(-> $ret)? => $trait for $macro!(...)
            $(; |$candidate| $is_bit_valid)?
        );
    };
    (
        $(-> $ret:ident)? => $trait:ident for $macro:ident!(...)
        $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        unsafe_impl_for_power_set!(
            @impl $(-> $ret)? => $trait for $macro!(...)
            $(; |$candidate| $is_bit_valid)?
        );
    };
    (
        @impl $($vars:ident),* $(-> $ret:ident)? => $trait:ident for $macro:ident!(...)
        $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        unsafe_impl!(
            $($vars,)* $($ret)? => $trait for $macro!($($vars),* $(-> $ret)?)
            $(; |$candidate| $is_bit_valid)?
        );
    };
}

/// Expands to an `Option<extern "C" fn>` type with the given argument types and
/// return type. Designed for use with `unsafe_impl_for_power_set`.
macro_rules! opt_extern_c_fn {
    ($($args:ident),* -> $ret:ident) => { Option<extern "C" fn($($args),*) -> $ret> };
}

/// Expands to a `Option<fn>` type with the given argument types and return
/// type. Designed for use with `unsafe_impl_for_power_set`.
macro_rules! opt_fn {
    ($($args:ident),* -> $ret:ident) => { Option<fn($($args),*) -> $ret> };
}

/// Implements trait(s) for a type or verifies the given implementation by
/// referencing an existing (derived) implementation.
///
/// This macro exists so that we can provide zerocopy-derive as an optional
/// dependency and still get the benefit of using its derives to validate that
/// our trait impls are sound.
///
/// When compiling without `--cfg 'feature = "derive"` and without `--cfg test`,
/// `impl_or_verify!` emits the provided trait impl. When compiling with either
/// of those cfgs, it is expected that the type in question is deriving the
/// traits instead. In this case, `impl_or_verify!` emits code which validates
/// that the given trait impl is at least as restrictive as the the impl emitted
/// by the custom derive. This has the effect of confirming that the impl which
/// is emitted when the `derive` feature is disabled is actually sound (on the
/// assumption that the impl emitted by the custom derive is sound).
///
/// The caller is still required to provide a safety comment (e.g. using the
/// `safety_comment!` macro) . The reason for this restriction is that, while
/// `impl_or_verify!` can guarantee that the provided impl is sound when it is
/// compiled with the appropriate cfgs, there is no way to guarantee that it is
/// ever compiled with those cfgs. In particular, it would be possible to
/// accidentally place an `impl_or_verify!` call in a context that is only ever
/// compiled when the `derive` feature is disabled. If that were to happen,
/// there would be nothing to prevent an unsound trait impl from being emitted.
/// Requiring a safety comment reduces the likelihood of emitting an unsound
/// impl in this case, and also provides useful documentation for readers of the
/// code.
///
/// Finally, if a `TryFromBytes::is_bit_valid` impl is provided, it must adhere
/// to the safety preconditions of [`unsafe_impl!`].
///
/// ## Example
///
/// ```rust,ignore
/// // Note that these derives are gated by `feature = "derive"`
/// #[cfg_attr(any(feature = "derive", test), derive(FromZeros, FromBytes, IntoBytes, Unaligned))]
/// #[repr(transparent)]
/// struct Wrapper<T>(T);
///
/// safety_comment! {
///     /// SAFETY:
///     /// `Wrapper<T>` is `repr(transparent)`, so it is sound to implement any
///     /// zerocopy trait if `T` implements that trait.
///     impl_or_verify!(T: FromZeros => FromZeros for Wrapper<T>);
///     impl_or_verify!(T: FromBytes => FromBytes for Wrapper<T>);
///     impl_or_verify!(T: IntoBytes => IntoBytes for Wrapper<T>);
///     impl_or_verify!(T: Unaligned => Unaligned for Wrapper<T>);
/// }
/// ```
macro_rules! impl_or_verify {
    // The following two match arms follow the same pattern as their
    // counterparts in `unsafe_impl!`; see the documentation on those arms for
    // more details.
    (
        const $constname:ident : $constty:ident $(,)?
        $($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?),*
        => $trait:ident for $ty:ty
    ) => {
        impl_or_verify!(@impl { unsafe_impl!(
            const $constname: $constty, $($tyvar $(: $(? $optbound +)* $($bound +)*)?),* => $trait for $ty
        ); });
        impl_or_verify!(@verify $trait, {
            impl<const $constname: $constty, $($tyvar $(: $(? $optbound +)* $($bound +)*)?),*> Subtrait for $ty {}
        });
    };
    (
        $($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?),*
        => $trait:ident for $ty:ty $(; |$candidate:ident| $is_bit_valid:expr)?
    ) => {
        impl_or_verify!(@impl { unsafe_impl!(
            $($tyvar $(: $(? $optbound +)* $($bound +)*)?),* => $trait for $ty
            $(; |$candidate| $is_bit_valid)?
        ); });
        impl_or_verify!(@verify $trait, {
            impl<$($tyvar $(: $(? $optbound +)* $($bound +)*)?),*> Subtrait for $ty {}
        });
    };
    (@impl $impl_block:tt) => {
        #[cfg(not(any(feature = "derive", test)))]
        const _: () = { $impl_block };
    };
    (@verify $trait:ident, $impl_block:tt) => {
        #[cfg(any(feature = "derive", test))]
        const _: () = {
            trait Subtrait: $trait {}
            $impl_block
        };
    };
}

/// Implements `KnownLayout` for a sized type.
macro_rules! impl_known_layout {
    ($(const $constvar:ident : $constty:ty, $tyvar:ident $(: ?$optbound:ident)? => $ty:ty),* $(,)?) => {
        $(impl_known_layout!(@inner const $constvar: $constty, $tyvar $(: ?$optbound)? => $ty);)*
    };
    ($($tyvar:ident $(: ?$optbound:ident)? => $ty:ty),* $(,)?) => {
        $(impl_known_layout!(@inner , $tyvar $(: ?$optbound)? => $ty);)*
    };
    ($($(#[$attrs:meta])* $ty:ty),*) => { $(impl_known_layout!(@inner , => $(#[$attrs])* $ty);)* };
    (@inner $(const $constvar:ident : $constty:ty)? , $($tyvar:ident $(: ?$optbound:ident)?)? => $(#[$attrs:meta])* $ty:ty) => {
        const _: () = {
            use core::ptr::NonNull;

            #[allow(non_local_definitions)]
            $(#[$attrs])*
            // SAFETY: Delegates safety to `DstLayout::for_type`.
            unsafe impl<$($tyvar $(: ?$optbound)?)? $(, const $constvar : $constty)?> KnownLayout for $ty {
                #[allow(clippy::missing_inline_in_public_items)]
                #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
                fn only_derive_is_allowed_to_implement_this_trait() where Self: Sized {}

                type PointerMetadata = ();

                // SAFETY: `CoreMaybeUninit<T>::LAYOUT` and `T::LAYOUT` are
                // identical because `CoreMaybeUninit<T>` has the same size and
                // alignment as `T` [1], and `CoreMaybeUninit` admits
                // uninitialized bytes in all positions.
                //
                // [1] Per https://doc.rust-lang.org/1.81.0/std/mem/union.MaybeUninit.html#layout-1:
                //
                //   `MaybeUninit<T>` is guaranteed to have the same size,
                //   alignment, and ABI as `T`
                type MaybeUninit = core::mem::MaybeUninit<Self>;

                const LAYOUT: crate::DstLayout = crate::DstLayout::for_type::<$ty>();

                // SAFETY: `.cast` preserves address and provenance.
                //
                // TODO(#429): Add documentation to `.cast` that promises that
                // it preserves provenance.
                #[inline(always)]
                fn raw_from_ptr_len(bytes: NonNull<u8>, _meta: ()) -> NonNull<Self> {
                    bytes.cast::<Self>()
                }

                #[inline(always)]
                fn pointer_to_metadata(_ptr: *mut Self) -> () {
                }
            }
        };
    };
}

/// Implements `KnownLayout` for a type in terms of the implementation of
/// another type with the same representation.
///
/// # Safety
///
/// - `$ty` and `$repr` must have the same:
///   - Fixed prefix size
///   - Alignment
///   - (For DSTs) trailing slice element size
/// - It must be valid to perform an `as` cast from `*mut $repr` to `*mut $ty`,
///   and this operation must preserve referent size (ie, `size_of_val_raw`).
macro_rules! unsafe_impl_known_layout {
    ($($tyvar:ident: ?Sized + KnownLayout =>)? #[repr($repr:ty)] $ty:ty) => {
        const _: () = {
            use core::ptr::NonNull;

            #[allow(non_local_definitions)]
            unsafe impl<$($tyvar: ?Sized + KnownLayout)?> KnownLayout for $ty {
                #[allow(clippy::missing_inline_in_public_items, dead_code)]
                #[cfg_attr(all(coverage_nightly, __ZEROCOPY_INTERNAL_USE_ONLY_NIGHTLY_FEATURES_IN_TESTS), coverage(off))]
                fn only_derive_is_allowed_to_implement_this_trait() {}

                type PointerMetadata = <$repr as KnownLayout>::PointerMetadata;
                type MaybeUninit = <$repr as KnownLayout>::MaybeUninit;

                const LAYOUT: DstLayout = <$repr as KnownLayout>::LAYOUT;

                // SAFETY: All operations preserve address and provenance.
                // Caller has promised that the `as` cast preserves size.
                //
                // TODO(#429): Add documentation to `NonNull::new_unchecked`
                // that it preserves provenance.
                #[inline(always)]
                fn raw_from_ptr_len(bytes: NonNull<u8>, meta: <$repr as KnownLayout>::PointerMetadata) -> NonNull<Self> {
                    #[allow(clippy::as_conversions)]
                    let ptr = <$repr>::raw_from_ptr_len(bytes, meta).as_ptr() as *mut Self;
                    // SAFETY: `ptr` was converted from `bytes`, which is non-null.
                    unsafe { NonNull::new_unchecked(ptr) }
                }

                #[inline(always)]
                fn pointer_to_metadata(ptr: *mut Self) -> Self::PointerMetadata {
                    #[allow(clippy::as_conversions)]
                    let ptr = ptr as *mut $repr;
                    <$repr>::pointer_to_metadata(ptr)
                }
            }
        };
    };
}

/// Uses `align_of` to confirm that a type or set of types have alignment 1.
///
/// Note that `align_of<T>` requires `T: Sized`, so this macro doesn't work for
/// unsized types.
macro_rules! assert_unaligned {
    ($($tys:ty),*) => {
        $(
            // We only compile this assertion under `cfg(test)` to avoid taking
            // an extra non-dev dependency (and making this crate more expensive
            // to compile for our dependents).
            #[cfg(test)]
            static_assertions::const_assert_eq!(core::mem::align_of::<$tys>(), 1);
        )*
    };
}

/// Emits a function definition as either `const fn` or `fn` depending on
/// whether the current toolchain version supports `const fn` with generic trait
/// bounds.
macro_rules! maybe_const_trait_bounded_fn {
    // This case handles both `self` methods (where `self` is by value) and
    // non-method functions. Each `$args` may optionally be followed by `:
    // $arg_tys:ty`, which can be omitted for `self`.
    ($(#[$attr:meta])* $vis:vis const fn $name:ident($($args:ident $(: $arg_tys:ty)?),* $(,)?) $(-> $ret_ty:ty)? $body:block) => {
        #[cfg(zerocopy_generic_bounds_in_const_fn_1_61_0)]
        $(#[$attr])* $vis const fn $name($($args $(: $arg_tys)?),*) $(-> $ret_ty)? $body

        #[cfg(not(zerocopy_generic_bounds_in_const_fn_1_61_0))]
        $(#[$attr])* $vis fn $name($($args $(: $arg_tys)?),*) $(-> $ret_ty)? $body
    };
}

/// Either panic (if the current Rust toolchain supports panicking in `const
/// fn`) or evaluate a constant that will cause an array indexing error whose
/// error message will include the format string.
///
/// The type that this expression evaluates to must be `Copy`, or else the
/// non-panicking desugaring will fail to compile.
macro_rules! const_panic {
    (@non_panic $($_arg:tt)+) => {{
        // This will type check to whatever type is expected based on the call
        // site.
        let panic: [_; 0] = [];
        // This will always fail (since we're indexing into an array of size 0.
        #[allow(unconditional_panic)]
        panic[0]
    }};
    ($($arg:tt)+) => {{
        #[cfg(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0)]
        panic!($($arg)+);
        #[cfg(not(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0))]
        const_panic!(@non_panic $($arg)+)
    }};
}

/// Either assert (if the current Rust toolchain supports panicking in `const
/// fn`) or evaluate the expression and, if it evaluates to `false`, call
/// `const_panic!`. This is used in place of `assert!` in const contexts to
/// accommodate old toolchains.
macro_rules! const_assert {
    ($e:expr) => {{
        #[cfg(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0)]
        assert!($e);
        #[cfg(not(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0))]
        {
            let e = $e;
            if !e {
                let _: () = const_panic!(@non_panic concat!("assertion failed: ", stringify!($e)));
            }
        }
    }};
    ($e:expr, $($args:tt)+) => {{
        #[cfg(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0)]
        assert!($e, $($args)+);
        #[cfg(not(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0))]
        {
            let e = $e;
            if !e {
                let _: () = const_panic!(@non_panic concat!("assertion failed: ", stringify!($e), ": ", stringify!($arg)), $($args)*);
            }
        }
    }};
}

/// Like `const_assert!`, but relative to `debug_assert!`.
macro_rules! const_debug_assert {
    ($e:expr $(, $msg:expr)?) => {{
        #[cfg(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0)]
        debug_assert!($e $(, $msg)?);
        #[cfg(not(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0))]
        {
            // Use this (rather than `#[cfg(debug_assertions)]`) to ensure that
            // `$e` is always compiled even if it will never be evaluated at
            // runtime.
            if cfg!(debug_assertions) {
                let e = $e;
                if !e {
                    let _: () = const_panic!(@non_panic concat!("assertion failed: ", stringify!($e) $(, ": ", $msg)?));
                }
            }
        }
    }}
}

/// Either invoke `unreachable!()` or `loop {}` depending on whether the Rust
/// toolchain supports panicking in `const fn`.
macro_rules! const_unreachable {
    () => {{
        #[cfg(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0)]
        unreachable!();

        #[cfg(not(zerocopy_panic_in_const_and_vec_try_reserve_1_57_0))]
        loop {}
    }};
}

/// Asserts at compile time that `$condition` is true for `Self` or the given
/// `$tyvar`s. Unlike `const_assert`, this is *strictly* a compile-time check;
/// it cannot be evaluated in a runtime context. The condition is checked after
/// monomorphization and, upon failure, emits a compile error.
macro_rules! static_assert {
    (Self $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )? => $condition:expr $(, $args:tt)*) => {{
        trait StaticAssert {
            const ASSERT: bool;
        }

        impl<T $(: $(? $optbound +)* $($bound +)*)?> StaticAssert for T {
            const ASSERT: bool = {
                const_assert!($condition $(, $args)*);
                $condition
            };
        }

        const_assert!(<Self as StaticAssert>::ASSERT);
    }};
    ($($tyvar:ident $(: $(? $optbound:ident $(+)?)* $($bound:ident $(+)?)* )?),* => $condition:expr $(, $args:tt)*) => {{
        trait StaticAssert {
            const ASSERT: bool;
        }

        impl<$($tyvar $(: $(? $optbound +)* $($bound +)*)?,)*> StaticAssert for ($($tyvar,)*) {
            const ASSERT: bool = {
                const_assert!($condition $(, $args)*);
                $condition
            };
        }

        const_assert!(<($($tyvar,)*) as StaticAssert>::ASSERT);
    }};
}

/// Assert at compile time that `tyvar` does not have a zero-sized DST
/// component.
macro_rules! static_assert_dst_is_not_zst {
    ($tyvar:ident) => {{
        use crate::KnownLayout;
        static_assert!($tyvar: ?Sized + KnownLayout => {
            let dst_is_zst = match $tyvar::LAYOUT.size_info {
                crate::SizeInfo::Sized { .. } => false,
                crate::SizeInfo::SliceDst(TrailingSliceLayout { elem_size, .. }) => {
                    elem_size == 0
                }
            };
            !dst_is_zst
        }, "cannot call this method on a dynamically-sized type whose trailing slice element is zero-sized");
    }}
}

macro_rules! cast {
    () => {
        |p| {
            // SAFETY: `NonNull::as_ptr` returns a non-null pointer, so the
            // argument to `NonNull::new_unchecked` is also non-null.
            #[allow(clippy::as_conversions, unused_unsafe)]
            #[allow(clippy::undocumented_unsafe_blocks)] // Clippy false positive
            return unsafe {
                core::ptr::NonNull::new_unchecked(core::ptr::NonNull::as_ptr(p) as *mut _)
            };
        }
    };
    ($p:ident) => {
        cast!()($p)
    };
}

/// Implements `TransmuteFrom` and `SizeEq` for `T` and `$wrapper<T>`.
///
/// # Safety
///
/// `T` and `$wrapper<T>` must have the same bit validity, and must have the
/// same size in the sense of `SizeEq`.
macro_rules! unsafe_impl_for_transparent_wrapper {
    (T $(: ?$optbound:ident)? => $wrapper:ident<T>) => {
        const _: () = {
            use core::ptr::NonNull;
            use crate::pointer::{TransmuteFrom, SizeEq, invariant::Valid};

            // SAFETY: The caller promises that `T` and `$wrapper<T>` have the
            // same bit validity.
            unsafe impl<T $(: ?$optbound)?> TransmuteFrom<T, Valid, Valid> for $wrapper<T> {}
            // SAFETY: See previous safety comment.
            unsafe impl<T $(: ?$optbound)?> TransmuteFrom<$wrapper<T>, Valid, Valid> for T {}
            // SAFETY: The caller promises that `T` and `$wrapper<T>` satisfy
            // `SizeEq`.
            unsafe impl<T $(: ?$optbound)?> SizeEq<T> for $wrapper<T> {
                fn cast_from_raw(t: NonNull<T>) -> NonNull<$wrapper<T>> {
                    cast!(t)
                }
            }
            // SAFETY: See previous safety comment.
            unsafe impl<T $(: ?$optbound)?> SizeEq<$wrapper<T>> for T {
                fn cast_from_raw(t: NonNull<$wrapper<T>>) -> NonNull<T> {
                    cast!(t)
                }
            }
        };

        // So that this macro must be invoked inside `safety_comment!` or else
        // it will generate a `clippy::undocumented_unsafe_blocks` warning.
        #[allow(unused_unsafe)]
        const _: () = unsafe {};
    };
}

macro_rules! impl_transitive_transmute_from {
    ($($tyvar:ident $(: ?$optbound:ident)?)? => $t:ty => $u:ty => $v:ty) => {
        const _: () = {
            use core::ptr::NonNull;
            use crate::pointer::{TransmuteFrom, SizeEq, invariant::Valid};

            // SAFETY: Since `$u: SizeEq<$t>` and `$v: SizeEq<U>`, this impl is
            // transitively sound.
            unsafe impl<$($tyvar $(: ?$optbound)?)?> SizeEq<$t> for $v
            where
                $u: SizeEq<$t>,
                $v: SizeEq<$u>,
            {
                fn cast_from_raw(t: NonNull<$t>) -> NonNull<$v> {
                    cast!(t)
                }
            }

            // SAFETY: Since `$u: TransmuteFrom<$t, Valid, Valid>`, it is sound
            // to transmute a bit-valid `$t` to a bit-valid `$u`. Since `$v:
            // TransmuteFrom<$u, Valid, Valid>`, it is sound to transmute that
            // bit-valid `$u` to a bit-valid `$v`.
            unsafe impl<$($tyvar $(: ?$optbound)?)?> TransmuteFrom<$t, Valid, Valid> for $v
            where
                $u: TransmuteFrom<$t, Valid, Valid>,
                $v: TransmuteFrom<$u, Valid, Valid>,
            {}
        };
    };
}

macro_rules! impl_size_eq {
    ($t:ty, $u:ty) => {
        const _: () = {
            use crate::pointer::SizeEq;
            use core::ptr::NonNull;

    static_assert!(=> mem::size_of::<$t>() == mem::size_of::<$u>());

            // SAFETY: We've asserted that their sizes are equal.
            unsafe impl SizeEq<$t> for $u {
                fn cast_from_raw(t: NonNull<$t>) -> NonNull<$u> {
                    cast!(t)
                }
            }
            // SAFETY: We've asserted that their sizes are equal.
            unsafe impl SizeEq<$u> for $t {
                fn cast_from_raw(u: NonNull<$u>) -> NonNull<$t> {
                    cast!(u)
                }
            }
        };
    };
}
