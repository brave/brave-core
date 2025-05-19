// Copyright 2024 The Fuchsia Authors
//
// Licensed under the 2-Clause BSD License <LICENSE-BSD or
// https://opensource.org/license/bsd-2-clause>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

use core::{
    cell::{Cell, UnsafeCell},
    mem::MaybeUninit as CoreMaybeUninit,
    ptr::NonNull,
};

use super::*;

safety_comment! {
    /// SAFETY:
    /// Per the reference [1], "the unit tuple (`()`) ... is guaranteed as a
    /// zero-sized type to have a size of 0 and an alignment of 1."
    /// - `Immutable`: `()` self-evidently does not contain any `UnsafeCell`s.
    /// - `TryFromBytes` (with no validator), `FromZeros`, `FromBytes`: There is
    ///   only one possible sequence of 0 bytes, and `()` is inhabited.
    /// - `IntoBytes`: Since `()` has size 0, it contains no padding bytes.
    /// - `Unaligned`: `()` has alignment 1.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/reference/type-layout.html#tuple-layout
    unsafe_impl!((): Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
    assert_unaligned!(());
}

safety_comment! {
    /// SAFETY:
    /// - `Immutable`: These types self-evidently do not contain any
    ///   `UnsafeCell`s.
    /// - `TryFromBytes` (with no validator), `FromZeros`, `FromBytes`: all bit
    ///   patterns are valid for numeric types [1]
    /// - `IntoBytes`: numeric types have no padding bytes [1]
    /// - `Unaligned` (`u8` and `i8` only): The reference [2] specifies the size
    ///   of `u8` and `i8` as 1 byte. We also know that:
    ///   - Alignment is >= 1 [3]
    ///   - Size is an integer multiple of alignment [4]
    ///   - The only value >= 1 for which 1 is an integer multiple is 1
    ///   Therefore, the only possible alignment for `u8` and `i8` is 1.
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/reference/types/numeric.html#bit-validity:
    ///
    ///     For every numeric type, `T`, the bit validity of `T` is equivalent to
    ///     the bit validity of `[u8; size_of::<T>()]`. An uninitialized byte is
    ///     not a valid `u8`.
    ///
    /// [2] https://doc.rust-lang.org/1.81.0/reference/type-layout.html#primitive-data-layout
    ///
    /// [3] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#size-and-alignment:
    ///
    ///     Alignment is measured in bytes, and must be at least 1.
    ///
    /// [4] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#size-and-alignment:
    ///
    ///     The size of a value is always a multiple of its alignment.
    ///
    /// TODO(#278): Once we've updated the trait docs to refer to `u8`s rather
    /// than bits or bytes, update this comment, especially the reference to
    /// [1].
    unsafe_impl!(u8: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
    unsafe_impl!(i8: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
    assert_unaligned!(u8, i8);
    unsafe_impl!(u16: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(i16: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(u32: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(i32: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(u64: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(i64: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(u128: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(i128: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(usize: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(isize: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(f32: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(f64: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    #[cfg(feature = "float-nightly")]
    unsafe_impl!(#[cfg_attr(doc_cfg, doc(cfg(feature = "float-nightly")))] f16: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
    #[cfg(feature = "float-nightly")]
    unsafe_impl!(#[cfg_attr(doc_cfg, doc(cfg(feature = "float-nightly")))] f128: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes);
}

safety_comment! {
    /// SAFETY:
    /// - `Immutable`: `bool` self-evidently does not contain any `UnsafeCell`s.
    /// - `FromZeros`: Valid since "[t]he value false has the bit pattern 0x00"
    ///   [1].
    /// - `IntoBytes`: Since "the boolean type has a size and alignment of 1
    ///   each" and "The value false has the bit pattern 0x00 and the value true
    ///   has the bit pattern 0x01" [1]. Thus, the only byte of the bool is
    ///   always initialized.
    /// - `Unaligned`: Per the reference [1], "[a]n object with the boolean type
    ///   has a size and alignment of 1 each."
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/reference/types/boolean.html
    unsafe_impl!(bool: Immutable, FromZeros, IntoBytes, Unaligned);
    assert_unaligned!(bool);
    /// SAFETY:
    /// The impl must only return `true` for its argument if the original
    /// `Maybe<bool>` refers to a valid `bool`. We only return true if the `u8`
    /// value is 0 or 1, and both of these are valid values for `bool` [1].
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/reference/types/boolean.html:
    ///
    ///   The value false has the bit pattern 0x00 and the value true has the
    ///   bit pattern 0x01.
    unsafe_impl!(=> TryFromBytes for bool; |byte| {
        let byte = byte.transmute::<u8, invariant::Valid, _>();
        *byte.unaligned_as_ref() < 2
    });
}

impl_size_eq!(bool, u8);

safety_comment! {
    /// SAFETY:
    /// - `Immutable`: `char` self-evidently does not contain any `UnsafeCell`s.
    /// - `FromZeros`: Per reference [1], "[a] value of type char is a Unicode
    ///   scalar value (i.e. a code point that is not a surrogate), represented
    ///   as a 32-bit unsigned word in the 0x0000 to 0xD7FF or 0xE000 to
    ///   0x10FFFF range" which contains 0x0000.
    /// - `IntoBytes`: `char` is per reference [1] "represented as a 32-bit
    ///   unsigned word" (`u32`) which is `IntoBytes`. Note that unlike `u32`,
    ///   not all bit patterns are valid for `char`.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/reference/types/textual.html
    unsafe_impl!(char: Immutable, FromZeros, IntoBytes);
    /// SAFETY:
    /// The impl must only return `true` for its argument if the original
    /// `Maybe<char>` refers to a valid `char`. `char::from_u32` guarantees that
    /// it returns `None` if its input is not a valid `char` [1].
    ///
    /// [1] Per https://doc.rust-lang.org/core/primitive.char.html#method.from_u32:
    ///
    ///   `from_u32()` will return `None` if the input is not a valid value for
    ///   a `char`.
    unsafe_impl!(=> TryFromBytes for char; |c| {
        let c = c.transmute::<Unalign<u32>, invariant::Valid, _>();
        let c = c.read_unaligned().into_inner();
        char::from_u32(c).is_some()
    });
}

impl_size_eq!(char, Unalign<u32>);

safety_comment! {
    /// SAFETY:
    /// Per the Reference [1], `str` has the same layout as `[u8]`.
    /// - `Immutable`: `[u8]` does not contain any `UnsafeCell`s.
    /// - `FromZeros`, `IntoBytes`, `Unaligned`: `[u8]` is `FromZeros`,
    ///   `IntoBytes`, and `Unaligned`.
    ///
    /// Note that we don't `assert_unaligned!(str)` because `assert_unaligned!`
    /// uses `align_of`, which only works for `Sized` types.
    ///
    /// TODO(#429):
    /// - Add quotes from documentation.
    /// - Improve safety proof for `FromZeros` and `IntoBytes`; having the same
    ///   layout as `[u8]` isn't sufficient.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/reference/type-layout.html#str-layout
    unsafe_impl!(str: Immutable, FromZeros, IntoBytes, Unaligned);
    /// SAFETY:
    /// The impl must only return `true` for its argument if the original
    /// `Maybe<str>` refers to a valid `str`. `str::from_utf8` guarantees that
    /// it returns `Err` if its input is not a valid `str` [1].
    ///
    /// [2] Per https://doc.rust-lang.org/core/str/fn.from_utf8.html#errors:
    ///
    ///   Returns `Err` if the slice is not UTF-8.
    unsafe_impl!(=> TryFromBytes for str; |c| {
        let c = c.transmute::<[u8], invariant::Valid, _>();
        let c = c.unaligned_as_ref();
        core::str::from_utf8(c).is_ok()
    });
}

// SAFETY: `str` and `[u8]` have the same layout [1].
//
// [1] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#str-layout:
//
//   String slices are a UTF-8 representation of characters that have the same
//   layout as slices of type `[u8]`.
unsafe impl pointer::SizeEq<str> for [u8] {
    fn cast_from_raw(s: NonNull<str>) -> NonNull<[u8]> {
        cast!(s)
    }
}
// SAFETY: See previous safety comment.
unsafe impl pointer::SizeEq<[u8]> for str {
    fn cast_from_raw(bytes: NonNull<[u8]>) -> NonNull<str> {
        cast!(bytes)
    }
}

macro_rules! unsafe_impl_try_from_bytes_for_nonzero {
    ($($nonzero:ident[$prim:ty]),*) => {
        $(
            unsafe_impl!(=> TryFromBytes for $nonzero; |n| {
                unsafe impl pointer::SizeEq<$nonzero> for Unalign<$prim> {
                    fn cast_from_raw(n: NonNull<$nonzero>) -> NonNull<Unalign<$prim>> {
                        cast!(n)
                    }
                }
                unsafe impl pointer::SizeEq<Unalign<$prim>> for $nonzero {
                    fn cast_from_raw(p: NonNull<Unalign<$prim>>) -> NonNull<$nonzero> {
                        cast!(p)
                    }
                }

                let n = n.transmute::<Unalign<$prim>, invariant::Valid, _>();
                $nonzero::new(n.read_unaligned().into_inner()).is_some()
            });
        )*
    }
}

safety_comment! {
    // `NonZeroXxx` is `IntoBytes`, but not `FromZeros` or `FromBytes`.
    //
    /// SAFETY:
    /// - `IntoBytes`: `NonZeroXxx` has the same layout as its associated
    ///    primitive. Since it is the same size, this guarantees it has no
    ///    padding - integers have no padding, and there's no room for padding
    ///    if it can represent all of the same values except 0.
    /// - `Unaligned`: `NonZeroU8` and `NonZeroI8` document that
    ///   `Option<NonZeroU8>` and `Option<NonZeroI8>` both have size 1. [1] [2]
    ///   This is worded in a way that makes it unclear whether it's meant as a
    ///   guarantee, but given the purpose of those types, it's virtually
    ///   unthinkable that that would ever change. `Option` cannot be smaller
    ///   than its contained type, which implies that, and `NonZeroX8` are of
    ///   size 1 or 0. `NonZeroX8` can represent multiple states, so they cannot
    ///   be 0 bytes, which means that they must be 1 byte. The only valid
    ///   alignment for a 1-byte type is 1.
    ///
    /// TODO(#429):
    /// - Add quotes from documentation.
    /// - Add safety comment for `Immutable`. How can we prove that `NonZeroXxx`
    ///   doesn't contain any `UnsafeCell`s? It's obviously true, but it's not
    ///   clear how we'd prove it short of adding text to the stdlib docs that
    ///   says so explicitly, which likely wouldn't be accepted.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/std/num/type.NonZeroU8.html
    ///
    ///     `NonZeroU8` is guaranteed to have the same layout and bit validity as `u8` with
    ///     the exception that 0 is not a valid instance
    ///
    /// [2] https://doc.rust-lang.org/1.81.0/std/num/type.NonZeroI8.html
    /// TODO(https://github.com/rust-lang/rust/pull/104082): Cite documentation
    /// that layout is the same as primitive layout.
    unsafe_impl!(NonZeroU8: Immutable, IntoBytes, Unaligned);
    unsafe_impl!(NonZeroI8: Immutable, IntoBytes, Unaligned);
    assert_unaligned!(NonZeroU8, NonZeroI8);
    unsafe_impl!(NonZeroU16: Immutable, IntoBytes);
    unsafe_impl!(NonZeroI16: Immutable, IntoBytes);
    unsafe_impl!(NonZeroU32: Immutable, IntoBytes);
    unsafe_impl!(NonZeroI32: Immutable, IntoBytes);
    unsafe_impl!(NonZeroU64: Immutable, IntoBytes);
    unsafe_impl!(NonZeroI64: Immutable, IntoBytes);
    unsafe_impl!(NonZeroU128: Immutable, IntoBytes);
    unsafe_impl!(NonZeroI128: Immutable, IntoBytes);
    unsafe_impl!(NonZeroUsize: Immutable, IntoBytes);
    unsafe_impl!(NonZeroIsize: Immutable, IntoBytes);
    unsafe_impl_try_from_bytes_for_nonzero!(
        NonZeroU8[u8],
        NonZeroI8[i8],
        NonZeroU16[u16],
        NonZeroI16[i16],
        NonZeroU32[u32],
        NonZeroI32[i32],
        NonZeroU64[u64],
        NonZeroI64[i64],
        NonZeroU128[u128],
        NonZeroI128[i128],
        NonZeroUsize[usize],
        NonZeroIsize[isize]
    );
}
safety_comment! {
    /// SAFETY:
    /// - `TryFromBytes` (with no validator), `FromZeros`, `FromBytes`,
    ///   `IntoBytes`: The Rust compiler reuses `0` value to represent `None`,
    ///   so `size_of::<Option<NonZeroXxx>>() == size_of::<xxx>()`; see
    ///   `NonZeroXxx` documentation.
    /// - `Unaligned`: `NonZeroU8` and `NonZeroI8` document that
    ///   `Option<NonZeroU8>` and `Option<NonZeroI8>` both have size 1. [1] [2]
    ///   This is worded in a way that makes it unclear whether it's meant as a
    ///   guarantee, but given the purpose of those types, it's virtually
    ///   unthinkable that that would ever change. The only valid alignment for
    ///   a 1-byte type is 1.
    ///
    /// TODO(#429): Add quotes from documentation.
    ///
    /// [1] https://doc.rust-lang.org/stable/std/num/struct.NonZeroU8.html
    /// [2] https://doc.rust-lang.org/stable/std/num/struct.NonZeroI8.html
    ///
    /// TODO(https://github.com/rust-lang/rust/pull/104082): Cite documentation
    /// for layout guarantees.
    unsafe_impl!(Option<NonZeroU8>: TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
    unsafe_impl!(Option<NonZeroI8>: TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
    assert_unaligned!(Option<NonZeroU8>, Option<NonZeroI8>);
    unsafe_impl!(Option<NonZeroU16>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroI16>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroU32>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroI32>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroU64>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroI64>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroU128>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroI128>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroUsize>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
    unsafe_impl!(Option<NonZeroIsize>: TryFromBytes, FromZeros, FromBytes, IntoBytes);
}

safety_comment! {
    /// SAFETY:
    /// While it's not fully documented, the consensus is that `Box<T>` does not
    /// contain any `UnsafeCell`s for `T: Sized` [1]. This is not a complete
    /// proof, but we are accepting this as a known risk per #1358.
    ///
    /// [1] https://github.com/rust-lang/unsafe-code-guidelines/issues/492
    #[cfg(feature = "alloc")]
    unsafe_impl!(
        #[cfg_attr(doc_cfg, doc(cfg(feature = "alloc")))]
        T: Sized => Immutable for Box<T>
    );
}

safety_comment! {
    /// SAFETY:
    /// The following types can be transmuted from `[0u8; size_of::<T>()]`. [1]
    ///
    /// [1] Per https://doc.rust-lang.org/nightly/core/option/index.html#representation:
    ///
    ///   Rust guarantees to optimize the following types `T` such that
    ///   [`Option<T>`] has the same size and alignment as `T`. In some of these
    ///   cases, Rust further guarantees that `transmute::<_, Option<T>>([0u8;
    ///   size_of::<T>()])` is sound and produces `Option::<T>::None`. These
    ///   cases are identified by the second column:
    ///
    ///   | `T`                   | `transmute::<_, Option<T>>([0u8; size_of::<T>()])` sound? |
    ///   |-----------------------|-----------------------------------------------------------|
    ///   | [`Box<U>`]            | when `U: Sized`                                           |
    ///   | `&U`                  | when `U: Sized`                                           |
    ///   | `&mut U`              | when `U: Sized`                                           |
    ///   | [`ptr::NonNull<U>`]   | when `U: Sized`                                           |
    ///   | `fn`, `extern "C" fn` | always                                                    |
    ///
    /// TODO(#429), TODO(https://github.com/rust-lang/rust/pull/115333): Cite
    /// the Stable docs once they're available.
    #[cfg(feature = "alloc")]
    unsafe_impl!(
        #[cfg_attr(doc_cfg, doc(cfg(feature = "alloc")))]
        T => TryFromBytes for Option<Box<T>>; |c| pointer::is_zeroed(c)
    );
    #[cfg(feature = "alloc")]
    unsafe_impl!(
        #[cfg_attr(doc_cfg, doc(cfg(feature = "alloc")))]
        T => FromZeros for Option<Box<T>>
    );
    unsafe_impl!(
        T => TryFromBytes for Option<&'_ T>; |c| pointer::is_zeroed(c)
    );
    unsafe_impl!(T => FromZeros for Option<&'_ T>);
    unsafe_impl!(
            T => TryFromBytes for Option<&'_ mut T>; |c| pointer::is_zeroed(c)
    );
    unsafe_impl!(T => FromZeros for Option<&'_ mut T>);
    unsafe_impl!(
        T => TryFromBytes for Option<NonNull<T>>; |c| pointer::is_zeroed(c)
    );
    unsafe_impl!(T => FromZeros for Option<NonNull<T>>);
    unsafe_impl_for_power_set!(A, B, C, D, E, F, G, H, I, J, K, L -> M => FromZeros for opt_fn!(...));
    unsafe_impl_for_power_set!(
        A, B, C, D, E, F, G, H, I, J, K, L -> M => TryFromBytes for opt_fn!(...);
        |c| pointer::is_zeroed(c)
    );
    unsafe_impl_for_power_set!(A, B, C, D, E, F, G, H, I, J, K, L -> M => FromZeros for opt_extern_c_fn!(...));
    unsafe_impl_for_power_set!(
        A, B, C, D, E, F, G, H, I, J, K, L -> M => TryFromBytes for opt_extern_c_fn!(...);
        |c| pointer::is_zeroed(c)
    );
}

safety_comment! {
    /// SAFETY:
    /// `fn()` and `extern "C" fn()` self-evidently do not contain
    /// `UnsafeCell`s. This is not a proof, but we are accepting this as a known
    /// risk per #1358.
    unsafe_impl_for_power_set!(A, B, C, D, E, F, G, H, I, J, K, L -> M => Immutable for opt_fn!(...));
    unsafe_impl_for_power_set!(A, B, C, D, E, F, G, H, I, J, K, L -> M => Immutable for opt_extern_c_fn!(...));
}

#[cfg(all(
    zerocopy_target_has_atomics_1_60_0,
    any(
        target_has_atomic = "8",
        target_has_atomic = "16",
        target_has_atomic = "32",
        target_has_atomic = "64",
        target_has_atomic = "ptr"
    )
))]
#[cfg_attr(doc_cfg, doc(cfg(rust = "1.60.0")))]
mod atomics {
    use super::*;

    macro_rules! impl_traits_for_atomics {
        ($($atomics:ident [$primitives:ident]),* $(,)?) => {
            $(
                impl_known_layout!($atomics);
                impl_for_transmute_from!(=> TryFromBytes for $atomics [UnsafeCell<$primitives>]);
                impl_for_transmute_from!(=> FromZeros for $atomics [UnsafeCell<$primitives>]);
                impl_for_transmute_from!(=> FromBytes for $atomics [UnsafeCell<$primitives>]);
                impl_for_transmute_from!(=> IntoBytes for $atomics [UnsafeCell<$primitives>]);
            )*
        };
    }

    /// Implements `TransmuteFrom` for `$atomic`, `$prim`, and
    /// `UnsafeCell<$prim>`.
    ///
    /// # Safety
    ///
    /// `$atomic` must have the same size and bit validity as `$prim`.
    macro_rules! unsafe_impl_transmute_from_for_atomic {
        ($($($tyvar:ident)? => $atomic:ty [$prim:ty]),*) => {
            const _: () = {
                use core::{cell::UnsafeCell, ptr::NonNull};
                use crate::pointer::{TransmuteFrom, SizeEq, invariant::Valid};

                $(
                    #[allow(unused_unsafe)] // Force the caller to call this macro inside `safety_comment!`.
                    const _: () = unsafe {};

                    // SAFETY: The caller promised that `$atomic` and `$prim` have
                    // the same size and bit validity.
                    unsafe impl<$($tyvar)?> TransmuteFrom<$atomic, Valid, Valid> for $prim {}
                    // SAFETY: The caller promised that `$atomic` and `$prim` have
                    // the same size and bit validity.
                    unsafe impl<$($tyvar)?> TransmuteFrom<$prim, Valid, Valid> for $atomic {}

                    // SAFETY: THe caller promised that `$atomic` and `$prim`
                    // have the same size.
                    unsafe impl<$($tyvar)?> SizeEq<$atomic> for $prim {
                        fn cast_from_raw(a: NonNull<$atomic>) -> NonNull<$prim> {
                            cast!(a)
                        }
                    }
                    // SAFETY: THe caller promised that `$atomic` and `$prim`
                    // have the same size.
                    unsafe impl<$($tyvar)?> SizeEq<$prim> for $atomic {
                        fn cast_from_raw(p: NonNull<$prim>) -> NonNull<$atomic> {
                            cast!(p)
                        }
                    }
                    // SAFETY: The caller promised that `$atomic` and `$prim`
                    // have the same size. `UnsafeCell<T>` has the same size as
                    // `T` [1].
                    //
                    // [1] Per https://doc.rust-lang.org/1.85.0/std/cell/struct.UnsafeCell.html#memory-layout:
                    //
                    //   `UnsafeCell<T>` has the same in-memory representation as
                    //   its inner type `T`. A consequence of this guarantee is that
                    //   it is possible to convert between `T` and `UnsafeCell<T>`.
                    unsafe impl<$($tyvar)?> SizeEq<$atomic> for UnsafeCell<$prim> {
                        fn cast_from_raw(a: NonNull<$atomic>) -> NonNull<UnsafeCell<$prim>> {
                            cast!(a)
                        }
                    }
                    // SAFETY: See previous safety comment.
                    unsafe impl<$($tyvar)?> SizeEq<UnsafeCell<$prim>> for $atomic {
                        fn cast_from_raw(p: NonNull<UnsafeCell<$prim>>) -> NonNull<$atomic> {
                            cast!(p)
                        }
                    }

                    // SAFETY: The caller promised that `$atomic` and `$prim`
                    // have the same bit validity. `UnsafeCell<T>` has the same
                    // bit validity as `T` [1].
                    //
                    // [1] Per https://doc.rust-lang.org/1.85.0/std/cell/struct.UnsafeCell.html#memory-layout:
                    //
                    //   `UnsafeCell<T>` has the same in-memory representation as
                    //   its inner type `T`. A consequence of this guarantee is that
                    //   it is possible to convert between `T` and `UnsafeCell<T>`.
                    unsafe impl<$($tyvar)?> TransmuteFrom<$atomic, Valid, Valid> for core::cell::UnsafeCell<$prim> {}
                    // SAFETY: See previous safety comment.
                    unsafe impl<$($tyvar)?> TransmuteFrom<core::cell::UnsafeCell<$prim>, Valid, Valid> for $atomic {}
                )*
            };
        };
    }

    #[cfg(target_has_atomic = "8")]
    #[cfg_attr(doc_cfg, doc(cfg(target_has_atomic = "8")))]
    mod atomic_8 {
        use core::sync::atomic::{AtomicBool, AtomicI8, AtomicU8};

        use super::*;

        impl_traits_for_atomics!(AtomicU8[u8], AtomicI8[i8]);

        impl_known_layout!(AtomicBool);

        impl_for_transmute_from!(=> TryFromBytes for AtomicBool [UnsafeCell<bool>]);
        impl_for_transmute_from!(=> FromZeros for AtomicBool [UnsafeCell<bool>]);
        impl_for_transmute_from!(=> IntoBytes for AtomicBool [UnsafeCell<bool>]);

        safety_comment! {
            /// SAFETY:
            /// Per [1], `AtomicBool`, `AtomicU8`, and `AtomicI8` have the same
            /// size as `bool`, `u8`, and `i8` respectively. Since a type's
            /// alignment cannot be smaller than 1 [2], and since its alignment
            /// cannot be greater than its size [3], the only possible value for
            /// the alignment is 1. Thus, it is sound to implement `Unaligned`.
            ///
            /// [1] Per (for example) https://doc.rust-lang.org/1.81.0/std/sync/atomic/struct.AtomicU8.html:
            ///
            ///   This type has the same size, alignment, and bit validity as
            ///   the underlying integer type
            ///
            /// [2] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#size-and-alignment:
            ///
            ///     Alignment is measured in bytes, and must be at least 1.
            ///
            /// [3] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#size-and-alignment:
            ///
            ///     The size of a value is always a multiple of its alignment.
            unsafe_impl!(AtomicBool: Unaligned);
            unsafe_impl!(AtomicU8: Unaligned);
            unsafe_impl!(AtomicI8: Unaligned);
            assert_unaligned!(AtomicBool, AtomicU8, AtomicI8);

            /// SAFETY:
            /// `AtomicU8`, `AtomicI8`, and `AtomicBool` have the same size and
            /// bit validity as `u8`, `i8`, and `bool` respectively [1][2][3].
            ///
            /// [1] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicU8.html:
            ///
            ///   This type has the same size, alignment, and bit validity as
            ///   the underlying integer type, `u8`.
            ///
            /// [2] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicI8.html:
            ///
            ///   This type has the same size, alignment, and bit validity as
            ///   the underlying integer type, `i8`.
            ///
            /// [3] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicBool.html:
            ///
            ///   This type has the same size, alignment, and bit validity a
            ///   `bool`.
            unsafe_impl_transmute_from_for_atomic!(
                => AtomicU8 [u8],
                => AtomicI8 [i8],
                => AtomicBool [bool]
            );
        }
    }

    #[cfg(target_has_atomic = "16")]
    #[cfg_attr(doc_cfg, doc(cfg(target_has_atomic = "16")))]
    mod atomic_16 {
        use core::sync::atomic::{AtomicI16, AtomicU16};

        use super::*;

        impl_traits_for_atomics!(AtomicU16[u16], AtomicI16[i16]);

        safety_comment! {
            /// SAFETY:
            /// `AtomicU16` and `AtomicI16` have the same size and bit validity
            /// as `u16` and `i16` respectively [1][2].
            ///
            /// [1] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicU16.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `u16`.
            ///
            /// [2] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicI16.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `i16`.
            unsafe_impl_transmute_from_for_atomic!(=> AtomicU16 [u16], => AtomicI16 [i16]);
        }
    }

    #[cfg(target_has_atomic = "32")]
    #[cfg_attr(doc_cfg, doc(cfg(target_has_atomic = "32")))]
    mod atomic_32 {
        use core::sync::atomic::{AtomicI32, AtomicU32};

        use super::*;

        impl_traits_for_atomics!(AtomicU32[u32], AtomicI32[i32]);

        safety_comment! {
            /// SAFETY:
            /// `AtomicU32` and `AtomicI32` have the same size and bit validity
            /// as `u32` and `i32` respectively [1][2].
            ///
            /// [1] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicU32.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `u32`.
            ///
            /// [2] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicI32.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `i32`.
            unsafe_impl_transmute_from_for_atomic!(=> AtomicU32 [u32], => AtomicI32 [i32]);
        }
    }

    #[cfg(target_has_atomic = "64")]
    #[cfg_attr(doc_cfg, doc(cfg(target_has_atomic = "64")))]
    mod atomic_64 {
        use core::sync::atomic::{AtomicI64, AtomicU64};

        use super::*;

        impl_traits_for_atomics!(AtomicU64[u64], AtomicI64[i64]);

        safety_comment! {
            /// SAFETY:
            /// `AtomicU64` and `AtomicI64` have the same size and bit validity
            /// as `u64` and `i64` respectively [1][2].
            ///
            /// [1] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicU64.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `u64`.
            ///
            /// [2] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicI64.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `i64`.
            unsafe_impl_transmute_from_for_atomic!(=> AtomicU64 [u64], => AtomicI64 [i64]);
        }
    }

    #[cfg(target_has_atomic = "ptr")]
    #[cfg_attr(doc_cfg, doc(cfg(target_has_atomic = "ptr")))]
    mod atomic_ptr {
        use core::sync::atomic::{AtomicIsize, AtomicPtr, AtomicUsize};

        use super::*;

        impl_traits_for_atomics!(AtomicUsize[usize], AtomicIsize[isize]);

        impl_known_layout!(T => AtomicPtr<T>);

        // TODO(#170): Implement `FromBytes` and `IntoBytes` once we implement
        // those traits for `*mut T`.
        impl_for_transmute_from!(T => TryFromBytes for AtomicPtr<T> [UnsafeCell<*mut T>]);
        impl_for_transmute_from!(T => FromZeros for AtomicPtr<T> [UnsafeCell<*mut T>]);

        safety_comment! {
            /// SAFETY:
            /// `AtomicUsize` and `AtomicIsize` have the same size and bit
            /// validity as `usize` and `isize` respectively [1][2].
            ///
            /// [1] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicUsize.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `usize`.
            ///
            /// [2] Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicIsize.html:
            ///
            ///   This type has the same size and bit validity as the underlying
            ///   integer type, `isize`.
            unsafe_impl_transmute_from_for_atomic!(=> AtomicUsize [usize], => AtomicIsize [isize]);
            /// SAFETY:
            /// Per https://doc.rust-lang.org/1.85.0/std/sync/atomic/struct.AtomicPtr.html:
            ///
            ///   This type has the same size and bit validity as a `*mut T`.
            unsafe_impl_transmute_from_for_atomic!(T => AtomicPtr<T> [*mut T]);
        }
    }
}

safety_comment! {
    /// SAFETY:
    /// Per reference [1]:
    /// "For all T, the following are guaranteed:
    /// size_of::<PhantomData<T>>() == 0
    /// align_of::<PhantomData<T>>() == 1".
    /// This gives:
    /// - `Immutable`: `PhantomData` has no fields.
    /// - `TryFromBytes` (with no validator), `FromZeros`, `FromBytes`: There is
    ///   only one possible sequence of 0 bytes, and `PhantomData` is inhabited.
    /// - `IntoBytes`: Since `PhantomData` has size 0, it contains no padding
    ///   bytes.
    /// - `Unaligned`: Per the preceding reference, `PhantomData` has alignment
    ///   1.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/std/marker/struct.PhantomData.html#layout-1
    unsafe_impl!(T: ?Sized => Immutable for PhantomData<T>);
    unsafe_impl!(T: ?Sized => TryFromBytes for PhantomData<T>);
    unsafe_impl!(T: ?Sized => FromZeros for PhantomData<T>);
    unsafe_impl!(T: ?Sized => FromBytes for PhantomData<T>);
    unsafe_impl!(T: ?Sized => IntoBytes for PhantomData<T>);
    unsafe_impl!(T: ?Sized => Unaligned for PhantomData<T>);
    assert_unaligned!(PhantomData<()>, PhantomData<u8>, PhantomData<u64>);
}

impl_for_transmute_from!(T: TryFromBytes => TryFromBytes for Wrapping<T>[<T>]);
impl_for_transmute_from!(T: FromZeros => FromZeros for Wrapping<T>[<T>]);
impl_for_transmute_from!(T: FromBytes => FromBytes for Wrapping<T>[<T>]);
impl_for_transmute_from!(T: IntoBytes => IntoBytes for Wrapping<T>[<T>]);
assert_unaligned!(Wrapping<()>, Wrapping<u8>);

safety_comment! {
    /// SAFETY:
    /// Per [1], `Wrapping<T>` has the same layout as `T`. Since its single
    /// field (of type `T`) is public, it would be a breaking change to add or
    /// remove fields. Thus, we know that `Wrapping<T>` contains a `T` (as
    /// opposed to just having the same size and alignment as `T`) with no pre-
    /// or post-padding. Thus, `Wrapping<T>` must have `UnsafeCell`s covering
    /// the same byte ranges as `Inner = T`.
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/std/num/struct.Wrapping.html#layout-1:
    ///
    ///   `Wrapping<T>` is guaranteed to have the same layout and ABI as `T`
    unsafe_impl!(T: Immutable => Immutable for Wrapping<T>);
    /// SAFETY:
    /// Per [1] in the preceding safety comment, `Wrapping<T>` has the same
    /// alignment as `T`.
    unsafe_impl!(T: Unaligned => Unaligned for Wrapping<T>);
}

safety_comment! {
    /// SAFETY:
    /// `TryFromBytes` (with no validator), `FromZeros`, `FromBytes`:
    /// `MaybeUninit<T>` has no restrictions on its contents.
    unsafe_impl!(T => TryFromBytes for CoreMaybeUninit<T>);
    unsafe_impl!(T => FromZeros for CoreMaybeUninit<T>);
    unsafe_impl!(T => FromBytes for CoreMaybeUninit<T>);
    /// SAFETY:
    /// `MaybeUninit<T>` has `UnsafeCell`s covering the same byte ranges as
    /// `Inner = T`. This is not explicitly documented, but it can be inferred.
    /// Per [1], `MaybeUninit<T>` has the same size as `T`. Further, note the
    /// signature of `MaybeUninit::assume_init_ref` [2]:
    ///
    ///   pub unsafe fn assume_init_ref(&self) -> &T
    ///
    /// If the argument `&MaybeUninit<T>` and the returned `&T` had
    /// `UnsafeCell`s at different offsets, this would be unsound. Its existence
    /// is proof that this is not the case.
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/std/mem/union.MaybeUninit.html#layout-1:
    ///
    /// `MaybeUninit<T>` is guaranteed to have the same size, alignment, and ABI
    /// as `T`.
    ///
    /// [2] https://doc.rust-lang.org/1.81.0/std/mem/union.MaybeUninit.html#method.assume_init_ref
    unsafe_impl!(T: Immutable => Immutable for CoreMaybeUninit<T>);
    /// SAFETY:
    /// Per [1] in the preceding safety comment, `MaybeUninit<T>` has the same
    /// alignment as `T`.
    unsafe_impl!(T: Unaligned => Unaligned for CoreMaybeUninit<T>);
}
assert_unaligned!(CoreMaybeUninit<()>, CoreMaybeUninit<u8>);

safety_comment! {
    /// SAFETY:
    /// `ManuallyDrop<T>` has the same layout as `T` [1]. This strongly implies,
    /// but does not guarantee, that it contains `UnsafeCell`s covering the same
    /// byte ranges as in `T`. However, it also implements `Defer<Target = T>`
    /// [2], which provides the ability to convert `&ManuallyDrop<T> -> &T`.
    /// This, combined with having the same size as `T`, implies that
    /// `ManuallyDrop<T>` exactly contains a `T` with the same fields and
    /// `UnsafeCell`s covering the same byte ranges, or else the `Deref` impl
    /// would permit safe code to obtain different shared references to the same
    /// region of memory with different `UnsafeCell` coverage, which would in
    /// turn permit interior mutation that would violate the invariants of a
    /// shared reference.
    ///
    /// [1] Per https://doc.rust-lang.org/1.85.0/std/mem/struct.ManuallyDrop.html:
    ///
    ///   `ManuallyDrop<T>` is guaranteed to have the same layout and bit
    ///   validity as `T`
    ///
    /// [2] https://doc.rust-lang.org/1.85.0/std/mem/struct.ManuallyDrop.html#impl-Deref-for-ManuallyDrop%3CT%3E
    unsafe_impl!(T: ?Sized + Immutable => Immutable for ManuallyDrop<T>);
}

impl_for_transmute_from!(T: ?Sized + TryFromBytes => TryFromBytes for ManuallyDrop<T>[<T>]);
impl_for_transmute_from!(T: ?Sized + FromZeros => FromZeros for ManuallyDrop<T>[<T>]);
impl_for_transmute_from!(T: ?Sized + FromBytes => FromBytes for ManuallyDrop<T>[<T>]);
impl_for_transmute_from!(T: ?Sized + IntoBytes => IntoBytes for ManuallyDrop<T>[<T>]);
safety_comment! {
    /// SAFETY:
    /// `ManuallyDrop<T>` has the same layout as `T` [1], and thus has the same
    /// alignment as `T`.
    ///
    /// [1] Per https://doc.rust-lang.org/nightly/core/mem/struct.ManuallyDrop.html:
    ///
    ///   `ManuallyDrop<T>` is guaranteed to have the same layout and bit
    ///   validity as `T`
    unsafe_impl!(T: ?Sized + Unaligned => Unaligned for ManuallyDrop<T>);
}
assert_unaligned!(ManuallyDrop<()>, ManuallyDrop<u8>);

impl_for_transmute_from!(T: ?Sized + TryFromBytes => TryFromBytes for Cell<T>[UnsafeCell<T>]);
impl_for_transmute_from!(T: ?Sized + FromZeros => FromZeros for Cell<T>[UnsafeCell<T>]);
impl_for_transmute_from!(T: ?Sized + FromBytes => FromBytes for Cell<T>[UnsafeCell<T>]);
impl_for_transmute_from!(T: ?Sized + IntoBytes => IntoBytes for Cell<T>[UnsafeCell<T>]);
safety_comment! {
    /// SAFETY:
    /// `Cell<T>` has the same in-memory representation as `T` [1], and thus has
    /// the same alignment as `T`.
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/core/cell/struct.Cell.html#memory-layout:
    ///
    ///   `Cell<T>` has the same in-memory representation as its inner type `T`.
    unsafe_impl!(T: ?Sized + Unaligned => Unaligned for Cell<T>);
}

impl_for_transmute_from!(T: ?Sized + FromZeros => FromZeros for UnsafeCell<T>[<T>]);
impl_for_transmute_from!(T: ?Sized + FromBytes => FromBytes for UnsafeCell<T>[<T>]);
impl_for_transmute_from!(T: ?Sized + IntoBytes => IntoBytes for UnsafeCell<T>[<T>]);
safety_comment! {
    /// SAFETY:
    /// `UnsafeCell<T>` has the same in-memory representation as `T` [1], and
    /// thus has the same alignment as `T`.
    ///
    /// [1] Per https://doc.rust-lang.org/1.81.0/core/cell/struct.UnsafeCell.html#memory-layout:
    ///
    ///   `UnsafeCell<T>` has the same in-memory representation as its inner
    ///   type `T`.
    unsafe_impl!(T: ?Sized + Unaligned => Unaligned for UnsafeCell<T>);
}
assert_unaligned!(UnsafeCell<()>, UnsafeCell<u8>);

// SAFETY: See safety comment in `is_bit_valid` impl.
unsafe impl<T: TryFromBytes + ?Sized> TryFromBytes for UnsafeCell<T> {
    #[allow(clippy::missing_inline_in_public_items)]
    fn only_derive_is_allowed_to_implement_this_trait()
    where
        Self: Sized,
    {
    }

    #[inline]
    fn is_bit_valid<A: invariant::Reference>(candidate: Maybe<'_, Self, A>) -> bool {
        // The only way to implement this function is using an exclusive-aliased
        // pointer. `UnsafeCell`s cannot be read via shared-aliased pointers
        // (other than by using `unsafe` code, which we can't use since we can't
        // guarantee how our users are accessing or modifying the `UnsafeCell`).
        //
        // `is_bit_valid` is documented as panicking or failing to monomorphize
        // if called with a shared-aliased pointer on a type containing an
        // `UnsafeCell`. In practice, it will always be a monorphization error.
        // Since `is_bit_valid` is `#[doc(hidden)]` and only called directly
        // from this crate, we only need to worry about our own code incorrectly
        // calling `UnsafeCell::is_bit_valid`. The post-monomorphization error
        // makes it easier to test that this is truly the case, and also means
        // that if we make a mistake, it will cause downstream code to fail to
        // compile, which will immediately surface the mistake and give us a
        // chance to fix it quickly.
        let c = candidate.into_exclusive_or_pme();

        // SAFETY: Since `UnsafeCell<T>` and `T` have the same layout and bit
        // validity, `UnsafeCell<T>` is bit-valid exactly when its wrapped `T`
        // is. Thus, this is a sound implementation of
        // `UnsafeCell::is_bit_valid`.
        T::is_bit_valid(c.get_mut())
    }
}

safety_comment! {
    /// SAFETY:
    /// Per the reference [1]:
    ///
    ///   An array of `[T; N]` has a size of `size_of::<T>() * N` and the same
    ///   alignment of `T`. Arrays are laid out so that the zero-based `nth`
    ///   element of the array is offset from the start of the array by `n *
    ///   size_of::<T>()` bytes.
    ///
    ///   ...
    ///
    ///   Slices have the same layout as the section of the array they slice.
    ///
    /// In other words, the layout of a `[T]` or `[T; N]` is a sequence of `T`s
    /// laid out back-to-back with no bytes in between. Therefore, `[T]` or `[T;
    /// N]` are `Immutable`, `TryFromBytes`, `FromZeros`, `FromBytes`, and
    /// `IntoBytes` if `T` is (respectively). Furthermore, since an array/slice
    /// has "the same alignment of `T`", `[T]` and `[T; N]` are `Unaligned` if
    /// `T` is.
    ///
    /// Note that we don't `assert_unaligned!` for slice types because
    /// `assert_unaligned!` uses `align_of`, which only works for `Sized` types.
    ///
    /// [1] https://doc.rust-lang.org/1.81.0/reference/type-layout.html#array-layout
    unsafe_impl!(const N: usize, T: Immutable => Immutable for [T; N]);
    unsafe_impl!(const N: usize, T: TryFromBytes => TryFromBytes for [T; N]; |c| {
        // Note that this call may panic, but it would still be sound even if it
        // did. `is_bit_valid` does not promise that it will not panic (in fact,
        // it explicitly warns that it's a possibility), and we have not
        // violated any safety invariants that we must fix before returning.
        <[T] as TryFromBytes>::is_bit_valid(c.as_slice())
    });
    unsafe_impl!(const N: usize, T: FromZeros => FromZeros for [T; N]);
    unsafe_impl!(const N: usize, T: FromBytes => FromBytes for [T; N]);
    unsafe_impl!(const N: usize, T: IntoBytes => IntoBytes for [T; N]);
    unsafe_impl!(const N: usize, T: Unaligned => Unaligned for [T; N]);
    assert_unaligned!([(); 0], [(); 1], [u8; 0], [u8; 1]);
    unsafe_impl!(T: Immutable => Immutable for [T]);
    unsafe_impl!(T: TryFromBytes => TryFromBytes for [T]; |c| {
        // SAFETY: Per the reference [1]:
        //
        //   An array of `[T; N]` has a size of `size_of::<T>() * N` and the
        //   same alignment of `T`. Arrays are laid out so that the zero-based
        //   `nth` element of the array is offset from the start of the array by
        //   `n * size_of::<T>()` bytes.
        //
        //   ...
        //
        //   Slices have the same layout as the section of the array they slice.
        //
        // In other words, the layout of a `[T] is a sequence of `T`s laid out
        // back-to-back with no bytes in between. If all elements in `candidate`
        // are `is_bit_valid`, so too is `candidate`.
        //
        // Note that any of the below calls may panic, but it would still be
        // sound even if it did. `is_bit_valid` does not promise that it will
        // not panic (in fact, it explicitly warns that it's a possibility), and
        // we have not violated any safety invariants that we must fix before
        // returning.
        c.iter().all(<T as TryFromBytes>::is_bit_valid)
    });
    unsafe_impl!(T: FromZeros => FromZeros for [T]);
    unsafe_impl!(T: FromBytes => FromBytes for [T]);
    unsafe_impl!(T: IntoBytes => IntoBytes for [T]);
    unsafe_impl!(T: Unaligned => Unaligned for [T]);
}
safety_comment! {
    /// SAFETY:
    /// - `Immutable`: Raw pointers do not contain any `UnsafeCell`s.
    /// - `FromZeros`: For thin pointers (note that `T: Sized`), the zero
    ///   pointer is considered "null". [1] No operations which require
    ///   provenance are legal on null pointers, so this is not a footgun.
    /// - `TryFromBytes`: By the same reasoning as for `FromZeroes`, we can
    ///   implement `TryFromBytes` for thin pointers provided that
    ///   [`TryFromByte::is_bit_valid`] only produces `true` for zeroed bytes.
    ///
    /// NOTE(#170): Implementing `FromBytes` and `IntoBytes` for raw pointers
    /// would be sound, but carries provenance footguns. We want to support
    /// `FromBytes` and `IntoBytes` for raw pointers eventually, but we are
    /// holding off until we can figure out how to address those footguns.
    ///
    /// [1] TODO(https://github.com/rust-lang/rust/pull/116988): Cite the
    /// documentation once this PR lands.
    unsafe_impl!(T: ?Sized => Immutable for *const T);
    unsafe_impl!(T: ?Sized => Immutable for *mut T);
    unsafe_impl!(T => TryFromBytes for *const T; |c| pointer::is_zeroed(c));
    unsafe_impl!(T => FromZeros for *const T);
    unsafe_impl!(T => TryFromBytes for *mut T; |c| pointer::is_zeroed(c));
    unsafe_impl!(T => FromZeros for *mut T);
}

safety_comment! {
    /// SAFETY:
    /// `NonNull<T>` self-evidently does not contain `UnsafeCell`s. This is not
    /// a proof, but we are accepting this as a known risk per #1358.
    unsafe_impl!(T: ?Sized => Immutable for NonNull<T>);
}

safety_comment! {
    /// SAFETY:
    /// Reference types do not contain any `UnsafeCell`s.
    unsafe_impl!(T: ?Sized => Immutable for &'_ T);
    unsafe_impl!(T: ?Sized => Immutable for &'_ mut T);
}

safety_comment! {
    /// SAFETY:
    /// `Option` is not `#[non_exhaustive]` [1], which means that the types in
    /// its variants cannot change, and no new variants can be added.
    /// `Option<T>` does not contain any `UnsafeCell`s outside of `T`. [1]
    ///
    /// [1] https://doc.rust-lang.org/core/option/enum.Option.html
    unsafe_impl!(T: Immutable => Immutable for Option<T>);
}

// SIMD support
//
// Per the Unsafe Code Guidelines Reference [1]:
//
//   Packed SIMD vector types are `repr(simd)` homogeneous tuple-structs
//   containing `N` elements of type `T` where `N` is a power-of-two and the
//   size and alignment requirements of `T` are equal:
//
//   ```rust
//   #[repr(simd)]
//   struct Vector<T, N>(T_0, ..., T_(N - 1));
//   ```
//
//   ...
//
//   The size of `Vector` is `N * size_of::<T>()` and its alignment is an
//   implementation-defined function of `T` and `N` greater than or equal to
//   `align_of::<T>()`.
//
//   ...
//
//   Vector elements are laid out in source field order, enabling random access
//   to vector elements by reinterpreting the vector as an array:
//
//   ```rust
//   union U {
//      vec: Vector<T, N>,
//      arr: [T; N]
//   }
//
//   assert_eq!(size_of::<Vector<T, N>>(), size_of::<[T; N]>());
//   assert!(align_of::<Vector<T, N>>() >= align_of::<[T; N]>());
//
//   unsafe {
//     let u = U { vec: Vector<T, N>(t_0, ..., t_(N - 1)) };
//
//     assert_eq!(u.vec.0, u.arr[0]);
//     // ...
//     assert_eq!(u.vec.(N - 1), u.arr[N - 1]);
//   }
//   ```
//
// Given this background, we can observe that:
// - The size and bit pattern requirements of a SIMD type are equivalent to the
//   equivalent array type. Thus, for any SIMD type whose primitive `T` is
//   `Immutable`, `TryFromBytes`, `FromZeros`, `FromBytes`, or `IntoBytes`, that
//   SIMD type is also `Immutable`, `TryFromBytes`, `FromZeros`, `FromBytes`, or
//   `IntoBytes` respectively.
// - Since no upper bound is placed on the alignment, no SIMD type can be
//   guaranteed to be `Unaligned`.
//
// Also per [1]:
//
//   This chapter represents the consensus from issue #38. The statements in
//   here are not (yet) "guaranteed" not to change until an RFC ratifies them.
//
// See issue #38 [2]. While this behavior is not technically guaranteed, the
// likelihood that the behavior will change such that SIMD types are no longer
// `TryFromBytes`, `FromZeros`, `FromBytes`, or `IntoBytes` is next to zero, as
// that would defeat the entire purpose of SIMD types. Nonetheless, we put this
// behavior behind the `simd` Cargo feature, which requires consumers to opt
// into this stability hazard.
//
// [1] https://rust-lang.github.io/unsafe-code-guidelines/layout/packed-simd-vectors.html
// [2] https://github.com/rust-lang/unsafe-code-guidelines/issues/38
#[cfg(feature = "simd")]
#[cfg_attr(doc_cfg, doc(cfg(feature = "simd")))]
mod simd {
    /// Defines a module which implements `TryFromBytes`, `FromZeros`,
    /// `FromBytes`, and `IntoBytes` for a set of types from a module in
    /// `core::arch`.
    ///
    /// `$arch` is both the name of the defined module and the name of the
    /// module in `core::arch`, and `$typ` is the list of items from that module
    /// to implement `FromZeros`, `FromBytes`, and `IntoBytes` for.
    #[allow(unused_macros)] // `allow(unused_macros)` is needed because some
                            // target/feature combinations don't emit any impls
                            // and thus don't use this macro.
    macro_rules! simd_arch_mod {
        (#[cfg $cfg:tt] $arch:ident, $mod:ident, $($typ:ident),*) => {
            #[cfg $cfg]
            #[cfg_attr(doc_cfg, doc(cfg $cfg))]
            mod $mod {
                use core::arch::$arch::{$($typ),*};

                use crate::*;
                impl_known_layout!($($typ),*);
                safety_comment! {
                    /// SAFETY:
                    /// See comment on module definition for justification.
                    $( unsafe_impl!($typ: Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes); )*
                }
            }
        };
    }

    #[rustfmt::skip]
    const _: () = {
        simd_arch_mod!(
            #[cfg(target_arch = "x86")]
            x86, x86, __m128, __m128d, __m128i, __m256, __m256d, __m256i
        );
        simd_arch_mod!(
            #[cfg(all(feature = "simd-nightly", target_arch = "x86"))]
            x86, x86_nightly, __m512bh, __m512, __m512d, __m512i
        );
        simd_arch_mod!(
            #[cfg(target_arch = "x86_64")]
            x86_64, x86_64, __m128, __m128d, __m128i, __m256, __m256d, __m256i
        );
        simd_arch_mod!(
            #[cfg(all(feature = "simd-nightly", target_arch = "x86_64"))]
            x86_64, x86_64_nightly, __m512bh, __m512, __m512d, __m512i
        );
        simd_arch_mod!(
            #[cfg(target_arch = "wasm32")]
            wasm32, wasm32, v128
        );
        simd_arch_mod!(
            #[cfg(all(feature = "simd-nightly", target_arch = "powerpc"))]
            powerpc, powerpc, vector_bool_long, vector_double, vector_signed_long, vector_unsigned_long
        );
        simd_arch_mod!(
            #[cfg(all(feature = "simd-nightly", target_arch = "powerpc64"))]
            powerpc64, powerpc64, vector_bool_long, vector_double, vector_signed_long, vector_unsigned_long
        );
        #[cfg(zerocopy_aarch64_simd_1_59_0)]
        #[cfg_attr(doc_cfg, doc(cfg(rust = "1.59.0")))]
        simd_arch_mod!(
            // NOTE(https://github.com/rust-lang/stdarch/issues/1484): NEON intrinsics are currently
            // broken on big-endian platforms.
            #[cfg(all(target_arch = "aarch64", target_endian = "little"))]
            aarch64, aarch64, float32x2_t, float32x4_t, float64x1_t, float64x2_t, int8x8_t, int8x8x2_t,
            int8x8x3_t, int8x8x4_t, int8x16_t, int8x16x2_t, int8x16x3_t, int8x16x4_t, int16x4_t,
            int16x8_t, int32x2_t, int32x4_t, int64x1_t, int64x2_t, poly8x8_t, poly8x8x2_t, poly8x8x3_t,
            poly8x8x4_t, poly8x16_t, poly8x16x2_t, poly8x16x3_t, poly8x16x4_t, poly16x4_t, poly16x8_t,
            poly64x1_t, poly64x2_t, uint8x8_t, uint8x8x2_t, uint8x8x3_t, uint8x8x4_t, uint8x16_t,
            uint8x16x2_t, uint8x16x3_t, uint8x16x4_t, uint16x4_t, uint16x8_t, uint32x2_t, uint32x4_t,
            uint64x1_t, uint64x2_t
        );
    };
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::pointer::invariant;

    #[test]
    fn test_impls() {
        // A type that can supply test cases for testing
        // `TryFromBytes::is_bit_valid`. All types passed to `assert_impls!`
        // must implement this trait; that macro uses it to generate runtime
        // tests for `TryFromBytes` impls.
        //
        // All `T: FromBytes` types are provided with a blanket impl. Other
        // types must implement `TryFromBytesTestable` directly (ie using
        // `impl_try_from_bytes_testable!`).
        trait TryFromBytesTestable {
            fn with_passing_test_cases<F: Fn(Box<Self>)>(f: F);
            fn with_failing_test_cases<F: Fn(&mut [u8])>(f: F);
        }

        impl<T: FromBytes> TryFromBytesTestable for T {
            fn with_passing_test_cases<F: Fn(Box<Self>)>(f: F) {
                // Test with a zeroed value.
                f(Self::new_box_zeroed().unwrap());

                let ffs = {
                    let mut t = Self::new_zeroed();
                    let ptr: *mut T = &mut t;
                    // SAFETY: `T: FromBytes`
                    unsafe { ptr::write_bytes(ptr.cast::<u8>(), 0xFF, mem::size_of::<T>()) };
                    t
                };

                // Test with a value initialized with 0xFF.
                f(Box::new(ffs));
            }

            fn with_failing_test_cases<F: Fn(&mut [u8])>(_f: F) {}
        }

        macro_rules! impl_try_from_bytes_testable_for_null_pointer_optimization {
            ($($tys:ty),*) => {
                $(
                    impl TryFromBytesTestable for Option<$tys> {
                        fn with_passing_test_cases<F: Fn(Box<Self>)>(f: F) {
                            // Test with a zeroed value.
                            f(Box::new(None));
                        }

                        fn with_failing_test_cases<F: Fn(&mut [u8])>(f: F) {
                            for pos in 0..mem::size_of::<Self>() {
                                let mut bytes = [0u8; mem::size_of::<Self>()];
                                bytes[pos] = 0x01;
                                f(&mut bytes[..]);
                            }
                        }
                    }
                )*
            };
        }

        // Implements `TryFromBytesTestable`.
        macro_rules! impl_try_from_bytes_testable {
            // Base case for recursion (when the list of types has run out).
            (=> @success $($success_case:expr),* $(, @failure $($failure_case:expr),*)?) => {};
            // Implements for type(s) with no type parameters.
            ($ty:ty $(,$tys:ty)* => @success $($success_case:expr),* $(, @failure $($failure_case:expr),*)?) => {
                impl TryFromBytesTestable for $ty {
                    impl_try_from_bytes_testable!(
                        @methods     @success $($success_case),*
                                 $(, @failure $($failure_case),*)?
                    );
                }
                impl_try_from_bytes_testable!($($tys),* => @success $($success_case),* $(, @failure $($failure_case),*)?);
            };
            // Implements for multiple types with no type parameters.
            ($($($ty:ty),* => @success $($success_case:expr), * $(, @failure $($failure_case:expr),*)?;)*) => {
                $(
                    impl_try_from_bytes_testable!($($ty),* => @success $($success_case),* $(, @failure $($failure_case),*)*);
                )*
            };
            // Implements only the methods; caller must invoke this from inside
            // an impl block.
            (@methods @success $($success_case:expr),* $(, @failure $($failure_case:expr),*)?) => {
                fn with_passing_test_cases<F: Fn(Box<Self>)>(_f: F) {
                    $(
                        _f(Box::<Self>::from($success_case));
                    )*
                }

                fn with_failing_test_cases<F: Fn(&mut [u8])>(_f: F) {
                    $($(
                        let mut case = $failure_case;
                        _f(case.as_mut_bytes());
                    )*)?
                }
            };
        }

        impl_try_from_bytes_testable_for_null_pointer_optimization!(
            Box<UnsafeCell<NotZerocopy>>,
            &'static UnsafeCell<NotZerocopy>,
            &'static mut UnsafeCell<NotZerocopy>,
            NonNull<UnsafeCell<NotZerocopy>>,
            fn(),
            FnManyArgs,
            extern "C" fn(),
            ECFnManyArgs
        );

        macro_rules! bx {
            ($e:expr) => {
                Box::new($e)
            };
        }

        // Note that these impls are only for types which are not `FromBytes`.
        // `FromBytes` types are covered by a preceding blanket impl.
        impl_try_from_bytes_testable!(
            bool => @success true, false,
                    @failure 2u8, 3u8, 0xFFu8;
            char => @success '\u{0}', '\u{D7FF}', '\u{E000}', '\u{10FFFF}',
                    @failure 0xD800u32, 0xDFFFu32, 0x110000u32;
            str  => @success "", "hello", "❤️🧡💛💚💙💜",
                    @failure [0, 159, 146, 150];
            [u8] => @success vec![].into_boxed_slice(), vec![0, 1, 2].into_boxed_slice();
            NonZeroU8, NonZeroI8, NonZeroU16, NonZeroI16, NonZeroU32,
            NonZeroI32, NonZeroU64, NonZeroI64, NonZeroU128, NonZeroI128,
            NonZeroUsize, NonZeroIsize
                => @success Self::new(1).unwrap(),
                   // Doing this instead of `0` ensures that we always satisfy
                   // the size and alignment requirements of `Self` (whereas `0`
                   // may be any integer type with a different size or alignment
                   // than some `NonZeroXxx` types).
                   @failure Option::<Self>::None;
            [bool; 0] => @success [];
            [bool; 1]
                => @success [true], [false],
                   @failure [2u8], [3u8], [0xFFu8];
            [bool]
                => @success vec![true, false].into_boxed_slice(), vec![false, true].into_boxed_slice(),
                    @failure [2u8], [3u8], [0xFFu8], [0u8, 1u8, 2u8];
            Unalign<bool>
                => @success Unalign::new(false), Unalign::new(true),
                   @failure 2u8, 0xFFu8;
            ManuallyDrop<bool>
                => @success ManuallyDrop::new(false), ManuallyDrop::new(true),
                   @failure 2u8, 0xFFu8;
            ManuallyDrop<[u8]>
                => @success bx!(ManuallyDrop::new([])), bx!(ManuallyDrop::new([0u8])), bx!(ManuallyDrop::new([0u8, 1u8]));
            ManuallyDrop<[bool]>
                => @success bx!(ManuallyDrop::new([])), bx!(ManuallyDrop::new([false])), bx!(ManuallyDrop::new([false, true])),
                   @failure [2u8], [3u8], [0xFFu8], [0u8, 1u8, 2u8];
            ManuallyDrop<[UnsafeCell<u8>]>
                => @success bx!(ManuallyDrop::new([UnsafeCell::new(0)])), bx!(ManuallyDrop::new([UnsafeCell::new(0), UnsafeCell::new(1)]));
            ManuallyDrop<[UnsafeCell<bool>]>
                => @success bx!(ManuallyDrop::new([UnsafeCell::new(false)])), bx!(ManuallyDrop::new([UnsafeCell::new(false), UnsafeCell::new(true)])),
                @failure [2u8], [3u8], [0xFFu8], [0u8, 1u8, 2u8];
            Wrapping<bool>
                => @success Wrapping(false), Wrapping(true),
                    @failure 2u8, 0xFFu8;
            *const NotZerocopy
                => @success ptr::null::<NotZerocopy>(),
                   @failure [0x01; mem::size_of::<*const NotZerocopy>()];
            *mut NotZerocopy
                => @success ptr::null_mut::<NotZerocopy>(),
                   @failure [0x01; mem::size_of::<*mut NotZerocopy>()];
        );

        // Use the trick described in [1] to allow us to call methods
        // conditional on certain trait bounds.
        //
        // In all of these cases, methods return `Option<R>`, where `R` is the
        // return type of the method we're conditionally calling. The "real"
        // implementations (the ones defined in traits using `&self`) return
        // `Some`, and the default implementations (the ones defined as inherent
        // methods using `&mut self`) return `None`.
        //
        // [1] https://github.com/dtolnay/case-studies/blob/master/autoref-specialization/README.md
        mod autoref_trick {
            use super::*;

            pub(super) struct AutorefWrapper<T: ?Sized>(pub(super) PhantomData<T>);

            pub(super) trait TestIsBitValidShared<T: ?Sized> {
                #[allow(clippy::needless_lifetimes)]
                fn test_is_bit_valid_shared<'ptr, A: invariant::Reference>(
                    &self,
                    candidate: Maybe<'ptr, T, A>,
                ) -> Option<bool>;
            }

            impl<T: TryFromBytes + Immutable + ?Sized> TestIsBitValidShared<T> for AutorefWrapper<T> {
                #[allow(clippy::needless_lifetimes)]
                fn test_is_bit_valid_shared<'ptr, A: invariant::Reference>(
                    &self,
                    candidate: Maybe<'ptr, T, A>,
                ) -> Option<bool> {
                    Some(T::is_bit_valid(candidate))
                }
            }

            pub(super) trait TestTryFromRef<T: ?Sized> {
                #[allow(clippy::needless_lifetimes)]
                fn test_try_from_ref<'bytes>(
                    &self,
                    bytes: &'bytes [u8],
                ) -> Option<Option<&'bytes T>>;
            }

            impl<T: TryFromBytes + Immutable + KnownLayout + ?Sized> TestTryFromRef<T> for AutorefWrapper<T> {
                #[allow(clippy::needless_lifetimes)]
                fn test_try_from_ref<'bytes>(
                    &self,
                    bytes: &'bytes [u8],
                ) -> Option<Option<&'bytes T>> {
                    Some(T::try_ref_from_bytes(bytes).ok())
                }
            }

            pub(super) trait TestTryFromMut<T: ?Sized> {
                #[allow(clippy::needless_lifetimes)]
                fn test_try_from_mut<'bytes>(
                    &self,
                    bytes: &'bytes mut [u8],
                ) -> Option<Option<&'bytes mut T>>;
            }

            impl<T: TryFromBytes + IntoBytes + KnownLayout + ?Sized> TestTryFromMut<T> for AutorefWrapper<T> {
                #[allow(clippy::needless_lifetimes)]
                fn test_try_from_mut<'bytes>(
                    &self,
                    bytes: &'bytes mut [u8],
                ) -> Option<Option<&'bytes mut T>> {
                    Some(T::try_mut_from_bytes(bytes).ok())
                }
            }

            pub(super) trait TestTryReadFrom<T> {
                fn test_try_read_from(&self, bytes: &[u8]) -> Option<Option<T>>;
            }

            impl<T: TryFromBytes> TestTryReadFrom<T> for AutorefWrapper<T> {
                fn test_try_read_from(&self, bytes: &[u8]) -> Option<Option<T>> {
                    Some(T::try_read_from_bytes(bytes).ok())
                }
            }

            pub(super) trait TestAsBytes<T: ?Sized> {
                #[allow(clippy::needless_lifetimes)]
                fn test_as_bytes<'slf, 't>(&'slf self, t: &'t T) -> Option<&'t [u8]>;
            }

            impl<T: IntoBytes + Immutable + ?Sized> TestAsBytes<T> for AutorefWrapper<T> {
                #[allow(clippy::needless_lifetimes)]
                fn test_as_bytes<'slf, 't>(&'slf self, t: &'t T) -> Option<&'t [u8]> {
                    Some(t.as_bytes())
                }
            }
        }

        use autoref_trick::*;

        // Asserts that `$ty` is one of a list of types which are allowed to not
        // provide a "real" implementation for `$fn_name`. Since the
        // `autoref_trick` machinery fails silently, this allows us to ensure
        // that the "default" impls are only being used for types which we
        // expect.
        //
        // Note that, since this is a runtime test, it is possible to have an
        // allowlist which is too restrictive if the function in question is
        // never called for a particular type. For example, if `as_bytes` is not
        // supported for a particular type, and so `test_as_bytes` returns
        // `None`, methods such as `test_try_from_ref` may never be called for
        // that type. As a result, it's possible that, for example, adding
        // `as_bytes` support for a type would cause other allowlist assertions
        // to fail. This means that allowlist assertion failures should not
        // automatically be taken as a sign of a bug.
        macro_rules! assert_on_allowlist {
            ($fn_name:ident($ty:ty) $(: $($tys:ty),*)?) => {{
                use core::any::TypeId;

                let allowlist: &[TypeId] = &[ $($(TypeId::of::<$tys>()),*)? ];
                let allowlist_names: &[&str] = &[ $($(stringify!($tys)),*)? ];

                let id = TypeId::of::<$ty>();
                assert!(allowlist.contains(&id), "{} is not on allowlist for {}: {:?}", stringify!($ty), stringify!($fn_name), allowlist_names);
            }};
        }

        // Asserts that `$ty` implements any `$trait` and doesn't implement any
        // `!$trait`. Note that all `$trait`s must come before any `!$trait`s.
        //
        // For `T: TryFromBytes`, uses `TryFromBytesTestable` to test success
        // and failure cases.
        macro_rules! assert_impls {
            ($ty:ty: TryFromBytes) => {
                // "Default" implementations that match the "real"
                // implementations defined in the `autoref_trick` module above.
                #[allow(unused, non_local_definitions)]
                impl AutorefWrapper<$ty> {
                    #[allow(clippy::needless_lifetimes)]
                    fn test_is_bit_valid_shared<'ptr, A: invariant::Reference>(
                        &mut self,
                        candidate: Maybe<'ptr, $ty, A>,
                    ) -> Option<bool> {
                        assert_on_allowlist!(
                            test_is_bit_valid_shared($ty):
                            ManuallyDrop<UnsafeCell<()>>,
                            ManuallyDrop<[UnsafeCell<u8>]>,
                            ManuallyDrop<[UnsafeCell<bool>]>,
                            CoreMaybeUninit<NotZerocopy>,
                            CoreMaybeUninit<UnsafeCell<()>>,
                            Wrapping<UnsafeCell<()>>
                        );

                        None
                    }

                    #[allow(clippy::needless_lifetimes)]
                    fn test_try_from_ref<'bytes>(&mut self, _bytes: &'bytes [u8]) -> Option<Option<&'bytes $ty>> {
                        assert_on_allowlist!(
                            test_try_from_ref($ty):
                            ManuallyDrop<[UnsafeCell<bool>]>
                        );

                        None
                    }

                    #[allow(clippy::needless_lifetimes)]
                    fn test_try_from_mut<'bytes>(&mut self, _bytes: &'bytes mut [u8]) -> Option<Option<&'bytes mut $ty>> {
                        assert_on_allowlist!(
                            test_try_from_mut($ty):
                            Option<Box<UnsafeCell<NotZerocopy>>>,
                            Option<&'static UnsafeCell<NotZerocopy>>,
                            Option<&'static mut UnsafeCell<NotZerocopy>>,
                            Option<NonNull<UnsafeCell<NotZerocopy>>>,
                            Option<fn()>,
                            Option<FnManyArgs>,
                            Option<extern "C" fn()>,
                            Option<ECFnManyArgs>,
                            *const NotZerocopy,
                            *mut NotZerocopy
                        );

                        None
                    }

                    fn test_try_read_from(&mut self, _bytes: &[u8]) -> Option<Option<&$ty>> {
                        assert_on_allowlist!(
                            test_try_read_from($ty):
                            str,
                            ManuallyDrop<[u8]>,
                            ManuallyDrop<[bool]>,
                            ManuallyDrop<[UnsafeCell<bool>]>,
                            [u8],
                            [bool]
                        );

                        None
                    }

                    fn test_as_bytes(&mut self, _t: &$ty) -> Option<&[u8]> {
                        assert_on_allowlist!(
                            test_as_bytes($ty):
                            Option<&'static UnsafeCell<NotZerocopy>>,
                            Option<&'static mut UnsafeCell<NotZerocopy>>,
                            Option<NonNull<UnsafeCell<NotZerocopy>>>,
                            Option<Box<UnsafeCell<NotZerocopy>>>,
                            Option<fn()>,
                            Option<FnManyArgs>,
                            Option<extern "C" fn()>,
                            Option<ECFnManyArgs>,
                            CoreMaybeUninit<u8>,
                            CoreMaybeUninit<NotZerocopy>,
                            CoreMaybeUninit<UnsafeCell<()>>,
                            ManuallyDrop<UnsafeCell<()>>,
                            ManuallyDrop<[UnsafeCell<u8>]>,
                            ManuallyDrop<[UnsafeCell<bool>]>,
                            Wrapping<UnsafeCell<()>>,
                            *const NotZerocopy,
                            *mut NotZerocopy
                        );

                        None
                    }
                }

                <$ty as TryFromBytesTestable>::with_passing_test_cases(|mut val| {
                    // TODO(#494): These tests only get exercised for types
                    // which are `IntoBytes`. Once we implement #494, we should
                    // be able to support non-`IntoBytes` types by zeroing
                    // padding.

                    // We define `w` and `ww` since, in the case of the inherent
                    // methods, Rust thinks they're both borrowed mutably at the
                    // same time (given how we use them below). If we just
                    // defined a single `w` and used it for multiple operations,
                    // this would conflict.
                    //
                    // We `#[allow(unused_mut]` for the cases where the "real"
                    // impls are used, which take `&self`.
                    #[allow(unused_mut)]
                    let (mut w, mut ww) = (AutorefWrapper::<$ty>(PhantomData), AutorefWrapper::<$ty>(PhantomData));

                    let c = Ptr::from_ref(&*val);
                    let c = c.forget_aligned();
                    // SAFETY: TODO(#899): This is unsound. `$ty` is not
                    // necessarily `IntoBytes`, but that's the corner we've
                    // backed ourselves into by using `Ptr::from_ref`.
                    let c = unsafe { c.assume_initialized() };
                    let res = w.test_is_bit_valid_shared(c);
                    if let Some(res) = res {
                        assert!(res, "{}::is_bit_valid({:?}) (shared `Ptr`): got false, expected true", stringify!($ty), val);
                    }

                    let c = Ptr::from_mut(&mut *val);
                    let c = c.forget_aligned();
                    // SAFETY: TODO(#899): This is unsound. `$ty` is not
                    // necessarily `IntoBytes`, but that's the corner we've
                    // backed ourselves into by using `Ptr::from_ref`.
                    let c = unsafe { c.assume_initialized() };
                    let res = <$ty as TryFromBytes>::is_bit_valid(c);
                    assert!(res, "{}::is_bit_valid({:?}) (exclusive `Ptr`): got false, expected true", stringify!($ty), val);

                    // `bytes` is `Some(val.as_bytes())` if `$ty: IntoBytes +
                    // Immutable` and `None` otherwise.
                    let bytes = w.test_as_bytes(&*val);

                    // The inner closure returns
                    // `Some($ty::try_ref_from_bytes(bytes))` if `$ty:
                    // Immutable` and `None` otherwise.
                    let res = bytes.and_then(|bytes| ww.test_try_from_ref(bytes));
                    if let Some(res) = res {
                        assert!(res.is_some(), "{}::try_ref_from_bytes({:?}): got `None`, expected `Some`", stringify!($ty), val);
                    }

                    if let Some(bytes) = bytes {
                        // We need to get a mutable byte slice, and so we clone
                        // into a `Vec`. However, we also need these bytes to
                        // satisfy `$ty`'s alignment requirement, which isn't
                        // guaranteed for `Vec<u8>`. In order to get around
                        // this, we create a `Vec` which is twice as long as we
                        // need. There is guaranteed to be an aligned byte range
                        // of size `size_of_val(val)` within that range.
                        let val = &*val;
                        let size = mem::size_of_val(val);
                        let align = mem::align_of_val(val);

                        let mut vec = bytes.to_vec();
                        vec.extend(bytes);
                        let slc = vec.as_slice();
                        let offset = slc.as_ptr().align_offset(align);
                        let bytes_mut = &mut vec.as_mut_slice()[offset..offset+size];
                        bytes_mut.copy_from_slice(bytes);

                        let res = ww.test_try_from_mut(bytes_mut);
                        if let Some(res) = res {
                            assert!(res.is_some(), "{}::try_mut_from_bytes({:?}): got `None`, expected `Some`", stringify!($ty), val);
                        }
                    }

                    let res = bytes.and_then(|bytes| ww.test_try_read_from(bytes));
                    if let Some(res) = res {
                        assert!(res.is_some(), "{}::try_read_from_bytes({:?}): got `None`, expected `Some`", stringify!($ty), val);
                    }
                });
                #[allow(clippy::as_conversions)]
                <$ty as TryFromBytesTestable>::with_failing_test_cases(|c| {
                    #[allow(unused_mut)] // For cases where the "real" impls are used, which take `&self`.
                    let mut w = AutorefWrapper::<$ty>(PhantomData);

                    // This is `Some($ty::try_ref_from_bytes(c))` if `$ty:
                    // Immutable` and `None` otherwise.
                    let res = w.test_try_from_ref(c);
                    if let Some(res) = res {
                        assert!(res.is_none(), "{}::try_ref_from_bytes({:?}): got Some, expected None", stringify!($ty), c);
                    }

                    let res = w.test_try_from_mut(c);
                    if let Some(res) = res {
                        assert!(res.is_none(), "{}::try_mut_from_bytes({:?}): got Some, expected None", stringify!($ty), c);
                    }


                    let res = w.test_try_read_from(c);
                    if let Some(res) = res {
                        assert!(res.is_none(), "{}::try_read_from_bytes({:?}): got Some, expected None", stringify!($ty), c);
                    }
                });

                #[allow(dead_code)]
                const _: () = { static_assertions::assert_impl_all!($ty: TryFromBytes); };
            };
            ($ty:ty: $trait:ident) => {
                #[allow(dead_code)]
                const _: () = { static_assertions::assert_impl_all!($ty: $trait); };
            };
            ($ty:ty: !$trait:ident) => {
                #[allow(dead_code)]
                const _: () = { static_assertions::assert_not_impl_any!($ty: $trait); };
            };
            ($ty:ty: $($trait:ident),* $(,)? $(!$negative_trait:ident),*) => {
                $(
                    assert_impls!($ty: $trait);
                )*

                $(
                    assert_impls!($ty: !$negative_trait);
                )*
            };
        }

        // NOTE: The negative impl assertions here are not necessarily
        // prescriptive. They merely serve as change detectors to make sure
        // we're aware of what trait impls are getting added with a given
        // change. Of course, some impls would be invalid (e.g., `bool:
        // FromBytes`), and so this change detection is very important.

        assert_impls!(
            (): KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned
        );
        assert_impls!(
            u8: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned
        );
        assert_impls!(
            i8: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned
        );
        assert_impls!(
            u16: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            i16: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            u32: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            i32: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            u64: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            i64: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            u128: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            i128: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            usize: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            isize: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        #[cfg(feature = "float-nightly")]
        assert_impls!(
            f16: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            f32: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            f64: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        #[cfg(feature = "float-nightly")]
        assert_impls!(
            f128: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            !Unaligned
        );
        assert_impls!(
            bool: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            IntoBytes,
            Unaligned,
            !FromBytes
        );
        assert_impls!(
            char: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            str: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            IntoBytes,
            Unaligned,
            !FromBytes
        );

        assert_impls!(
            NonZeroU8: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            Unaligned,
            !FromZeros,
            !FromBytes
        );
        assert_impls!(
            NonZeroI8: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            Unaligned,
            !FromZeros,
            !FromBytes
        );
        assert_impls!(
            NonZeroU16: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroI16: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroU32: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroI32: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroU64: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroI64: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroU128: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroI128: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroUsize: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );
        assert_impls!(
            NonZeroIsize: KnownLayout,
            Immutable,
            TryFromBytes,
            IntoBytes,
            !FromBytes,
            !Unaligned
        );

        assert_impls!(Option<NonZeroU8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        assert_impls!(Option<NonZeroI8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        assert_impls!(Option<NonZeroU16>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroI16>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroU32>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroI32>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroU64>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroI64>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroU128>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroI128>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroUsize>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);
        assert_impls!(Option<NonZeroIsize>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned);

        // Implements none of the ZC traits.
        struct NotZerocopy;

        #[rustfmt::skip]
        type FnManyArgs = fn(
            NotZerocopy, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
        ) -> (NotZerocopy, NotZerocopy);

        // Allowed, because we're not actually using this type for FFI.
        #[allow(improper_ctypes_definitions)]
        #[rustfmt::skip]
        type ECFnManyArgs = extern "C" fn(
            NotZerocopy, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8, u8,
        ) -> (NotZerocopy, NotZerocopy);

        #[cfg(feature = "alloc")]
        assert_impls!(Option<Box<UnsafeCell<NotZerocopy>>>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<Box<[UnsafeCell<NotZerocopy>]>>: KnownLayout, !Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<&'static UnsafeCell<NotZerocopy>>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<&'static [UnsafeCell<NotZerocopy>]>: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<&'static mut UnsafeCell<NotZerocopy>>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<&'static mut [UnsafeCell<NotZerocopy>]>: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<NonNull<UnsafeCell<NotZerocopy>>>: KnownLayout, TryFromBytes, FromZeros, Immutable, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<NonNull<[UnsafeCell<NotZerocopy>]>>: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<fn()>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<FnManyArgs>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<extern "C" fn()>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Option<ECFnManyArgs>: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);

        assert_impls!(PhantomData<NotZerocopy>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        assert_impls!(PhantomData<UnsafeCell<()>>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        assert_impls!(PhantomData<[u8]>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);

        assert_impls!(ManuallyDrop<u8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        // This test is important because it allows us to test our hand-rolled
        // implementation of `<ManuallyDrop<T> as TryFromBytes>::is_bit_valid`.
        assert_impls!(ManuallyDrop<bool>: KnownLayout, Immutable, TryFromBytes, FromZeros, IntoBytes, Unaligned, !FromBytes);
        assert_impls!(ManuallyDrop<[u8]>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        // This test is important because it allows us to test our hand-rolled
        // implementation of `<ManuallyDrop<T> as TryFromBytes>::is_bit_valid`.
        assert_impls!(ManuallyDrop<[bool]>: KnownLayout, Immutable, TryFromBytes, FromZeros, IntoBytes, Unaligned, !FromBytes);
        assert_impls!(ManuallyDrop<NotZerocopy>: !Immutable, !TryFromBytes, !KnownLayout, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(ManuallyDrop<[NotZerocopy]>: KnownLayout, !Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(ManuallyDrop<UnsafeCell<()>>: KnownLayout, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned, !Immutable);
        assert_impls!(ManuallyDrop<[UnsafeCell<u8>]>: KnownLayout, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned, !Immutable);
        assert_impls!(ManuallyDrop<[UnsafeCell<bool>]>: KnownLayout, TryFromBytes, FromZeros, IntoBytes, Unaligned, !Immutable, !FromBytes);

        assert_impls!(CoreMaybeUninit<u8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, Unaligned, !IntoBytes);
        assert_impls!(CoreMaybeUninit<NotZerocopy>: KnownLayout, TryFromBytes, FromZeros, FromBytes, !Immutable, !IntoBytes, !Unaligned);
        assert_impls!(CoreMaybeUninit<UnsafeCell<()>>: KnownLayout, TryFromBytes, FromZeros, FromBytes, Unaligned, !Immutable, !IntoBytes);

        assert_impls!(Wrapping<u8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        // This test is important because it allows us to test our hand-rolled
        // implementation of `<Wrapping<T> as TryFromBytes>::is_bit_valid`.
        assert_impls!(Wrapping<bool>: KnownLayout, Immutable, TryFromBytes, FromZeros, IntoBytes, Unaligned, !FromBytes);
        assert_impls!(Wrapping<NotZerocopy>: KnownLayout, !Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(Wrapping<UnsafeCell<()>>: KnownLayout, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned, !Immutable);

        assert_impls!(Unalign<u8>: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, Unaligned);
        // This test is important because it allows us to test our hand-rolled
        // implementation of `<Unalign<T> as TryFromBytes>::is_bit_valid`.
        assert_impls!(Unalign<bool>: KnownLayout, Immutable, TryFromBytes, FromZeros, IntoBytes, Unaligned, !FromBytes);
        assert_impls!(Unalign<NotZerocopy>: KnownLayout, Unaligned, !Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes);

        assert_impls!(
            [u8]: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned
        );
        assert_impls!(
            [bool]: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            IntoBytes,
            Unaligned,
            !FromBytes
        );
        assert_impls!([NotZerocopy]: KnownLayout, !Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(
            [u8; 0]: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned,
        );
        assert_impls!(
            [NotZerocopy; 0]: KnownLayout,
            !Immutable,
            !TryFromBytes,
            !FromZeros,
            !FromBytes,
            !IntoBytes,
            !Unaligned
        );
        assert_impls!(
            [u8; 1]: KnownLayout,
            Immutable,
            TryFromBytes,
            FromZeros,
            FromBytes,
            IntoBytes,
            Unaligned,
        );
        assert_impls!(
            [NotZerocopy; 1]: KnownLayout,
            !Immutable,
            !TryFromBytes,
            !FromZeros,
            !FromBytes,
            !IntoBytes,
            !Unaligned
        );

        assert_impls!(*const NotZerocopy: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(*mut NotZerocopy: KnownLayout, Immutable, TryFromBytes, FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(*const [NotZerocopy]: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(*mut [NotZerocopy]: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(*const dyn Debug: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);
        assert_impls!(*mut dyn Debug: KnownLayout, Immutable, !TryFromBytes, !FromZeros, !FromBytes, !IntoBytes, !Unaligned);

        #[cfg(feature = "simd")]
        {
            #[allow(unused_macros)]
            macro_rules! test_simd_arch_mod {
                ($arch:ident, $($typ:ident),*) => {
                    {
                        use core::arch::$arch::{$($typ),*};
                        use crate::*;
                        $( assert_impls!($typ: KnownLayout, Immutable, TryFromBytes, FromZeros, FromBytes, IntoBytes, !Unaligned); )*
                    }
                };
            }
            #[cfg(target_arch = "x86")]
            test_simd_arch_mod!(x86, __m128, __m128d, __m128i, __m256, __m256d, __m256i);

            #[cfg(all(feature = "simd-nightly", target_arch = "x86"))]
            test_simd_arch_mod!(x86, __m512bh, __m512, __m512d, __m512i);

            #[cfg(target_arch = "x86_64")]
            test_simd_arch_mod!(x86_64, __m128, __m128d, __m128i, __m256, __m256d, __m256i);

            #[cfg(all(feature = "simd-nightly", target_arch = "x86_64"))]
            test_simd_arch_mod!(x86_64, __m512bh, __m512, __m512d, __m512i);

            #[cfg(target_arch = "wasm32")]
            test_simd_arch_mod!(wasm32, v128);

            #[cfg(all(feature = "simd-nightly", target_arch = "powerpc"))]
            test_simd_arch_mod!(
                powerpc,
                vector_bool_long,
                vector_double,
                vector_signed_long,
                vector_unsigned_long
            );

            #[cfg(all(feature = "simd-nightly", target_arch = "powerpc64"))]
            test_simd_arch_mod!(
                powerpc64,
                vector_bool_long,
                vector_double,
                vector_signed_long,
                vector_unsigned_long
            );
            #[cfg(all(target_arch = "aarch64", zerocopy_aarch64_simd_1_59_0))]
            #[rustfmt::skip]
            test_simd_arch_mod!(
                aarch64, float32x2_t, float32x4_t, float64x1_t, float64x2_t, int8x8_t, int8x8x2_t,
                int8x8x3_t, int8x8x4_t, int8x16_t, int8x16x2_t, int8x16x3_t, int8x16x4_t, int16x4_t,
                int16x8_t, int32x2_t, int32x4_t, int64x1_t, int64x2_t, poly8x8_t, poly8x8x2_t, poly8x8x3_t,
                poly8x8x4_t, poly8x16_t, poly8x16x2_t, poly8x16x3_t, poly8x16x4_t, poly16x4_t, poly16x8_t,
                poly64x1_t, poly64x2_t, uint8x8_t, uint8x8x2_t, uint8x8x3_t, uint8x8x4_t, uint8x16_t,
                uint8x16x2_t, uint8x16x3_t, uint8x16x4_t, uint16x4_t, uint16x8_t, uint32x2_t, uint32x4_t,
                uint64x1_t, uint64x2_t
            );
        }
    }
}
