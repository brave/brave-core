// Copyright 2019 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

// See comment in `include.rs` for why we disable the prelude.
#![no_implicit_prelude]
#![allow(warnings)]

include!("include.rs");

// A struct can derive `IntoBytes` if all of its fields implement `IntoBytes`
// and its representation lets the derive prove that the struct introduces no
// uninitialized outer padding. The cases below are grouped by representation
// and by the proof strategy used by the derive.

//
// Non-generic `repr(C)`
//

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct CZst;

util_assert_impl_all!(CZst: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(8))]
struct ReprCAlignedZst;

util_assert_impl_all!(ReprCAlignedZst: imp::IntoBytes);

#[derive(imp::IntoBytes, Copy, Clone)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(8))]
struct ReprCAligned8([u8; 8]);

util_assert_impl_all!(ReprCAligned8: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct C {
    a: u8,
    b: u8,
    c: util::AU16,
}

util_assert_impl_all!(C: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCSyntacticDst {
    a: u8,
    b: u8,
    c: [util::AU16],
}

util_assert_impl_all!(ReprCSyntacticDst: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(2))]
struct ReprCSyntacticDstAligned {
    a: [[u8; 2]],
}

util_assert_impl_all!(ReprCSyntacticDstAligned: imp::IntoBytes);

//
// Generic `repr(C)`
//

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCGenericOneField<T: ?imp::Sized> {
    t: T,
}

// Even though `ReprCGenericOneField` has generic type arguments, its one-field
// layout never requires an `Unaligned` bound. Sized, array, and slice
// instantiations are asserted in `assert_repr_c_homogeneous` below.

// Given `T: Sized + IntoBytes`, a plain `repr(C)` struct whose fields are all
// `T`, `[T; N]`, or a final `[T]` introduces no outer padding. `T` and all of
// its arrays and slices have the same alignment. The size of `T`, every array,
// and every slice value is a multiple of that alignment; bytes within each `T`
// are covered by `T`'s `IntoBytes` implementation.
//
// The following six declarations exhaustively enumerate the direct two-field
// shape combinations in this grammar. A slice may only be the final field, so
// the first field is either `T` or `[T; N]`, while the second is `T`, `[T; M]`,
// or `[T]`. The one-field case is covered by `ReprCGenericOneField` above.

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousTThenT<T>(T, T);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousTThenArray<T, const N: usize>(T, [T; N]);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousArrayThenT<T, const N: usize>([T; N], T);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousArrayThenArray<T, const N: usize, const M: usize>([T; N], [T; M]);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousTThenSlice<T>(T, [T]);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousArrayThenSlice<T, const N: usize>([T; N], [T]);

// Exercise longer named-field structs containing every allowed sized field
// shape, both with and without a trailing slice. Together with the two-field
// matrix above, these ensure that the derive handles repeated transitions and
// both of its sized/DST paths rather than special-casing a particular arity.
// The literal-length field also ensures that recognition does not depend on
// the array length being a const parameter.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousMixedSized<T, const N: usize, const M: usize> {
    literal: [T; 0],
    array0: [T; N],
    t0: T,
    array1: [T; M],
    t1: T,
}

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHomogeneousMixedDst<T, const N: usize, const M: usize> {
    t0: T,
    array0: [T; N],
    t1: T,
    array1: [T; M],
    tail: [T],
}

// `align(1)` cannot increase a type's alignment, so it preserves the same
// no-padding argument as an unmodified `repr(C)`. Test both the sized and DST
// paths.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(1))]
struct ReprCHomogeneousAlign1Sized<T, const N: usize>(T, [T; N], T);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(1))]
struct ReprCHomogeneousAlign1Dst<T, const N: usize>(T, [T; N], [T]);

// `packed(2)` caps every field's alignment at the same value. Since every
// field's size is still a multiple of that capped alignment, it likewise
// preserves the no-padding argument. A packed DST may only contain elements
// which do not need drop, so the DST declaration additionally requires
// `Copy`.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed(2))]
struct ReprCHomogeneousPacked2Sized<T, const N: usize>(T, [T; N], T);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed(2))]
struct ReprCHomogeneousPacked2Dst<T: imp::Copy, const N: usize>(T, [T; N], [T]);

// This generic assertion is stronger than checking a finite set of concrete
// element types and array lengths: its body only assumes `T: IntoBytes`, and
// is type-checked for every `N` and `M`.
fn assert_repr_c_homogeneous<T: imp::IntoBytes, const N: usize, const M: usize>() {
    fn assert_into_bytes<T: ?imp::Sized + imp::IntoBytes>() {}

    assert_into_bytes::<ReprCGenericOneField<T>>();
    assert_into_bytes::<ReprCGenericOneField<[T; N]>>();
    assert_into_bytes::<ReprCGenericOneField<[T]>>();
    assert_into_bytes::<ReprCHomogeneousTThenT<T>>();
    assert_into_bytes::<ReprCHomogeneousTThenArray<T, N>>();
    assert_into_bytes::<ReprCHomogeneousArrayThenT<T, N>>();
    assert_into_bytes::<ReprCHomogeneousArrayThenArray<T, N, M>>();
    assert_into_bytes::<ReprCHomogeneousTThenSlice<T>>();
    assert_into_bytes::<ReprCHomogeneousArrayThenSlice<T, N>>();
    assert_into_bytes::<ReprCHomogeneousMixedSized<T, N, M>>();
    assert_into_bytes::<ReprCHomogeneousMixedDst<T, N, M>>();
    assert_into_bytes::<ReprCHomogeneousAlign1Sized<T, N>>();
    assert_into_bytes::<ReprCHomogeneousAlign1Dst<T, N>>();
    assert_into_bytes::<ReprCHomogeneousPacked2Sized<T, N>>();
}

fn assert_repr_c_homogeneous_packed_dst<T: imp::Copy + imp::IntoBytes, const N: usize>() {
    fn assert_into_bytes<T: ?imp::Sized + imp::IntoBytes>() {}

    assert_into_bytes::<ReprCHomogeneousPacked2Dst<T, N>>();
}

// `AU16` is guaranteed to have alignment 2 and not implement `Unaligned`, so
// these assertions distinguish the homogeneous case from the existing
// `Unaligned` fallback. The witnesses cover every field shape, zero and
// nonzero arrays in both positions, and both the sized and DST paths.
util_assert_impl_all!(ReprCHomogeneousTThenT<util::AU16>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousTThenArray<util::AU16, 0>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousArrayThenT<util::AU16, 3>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousArrayThenArray<util::AU16, 0, 3>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousTThenSlice<util::AU16>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousArrayThenSlice<util::AU16, 0>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousArrayThenSlice<util::AU16, 3>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousMixedSized<util::AU16, 0, 3>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousMixedDst<util::AU16, 0, 3>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousAlign1Sized<util::AU16, 0>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousAlign1Dst<util::AU16, 3>: imp::IntoBytes);
// This exercises the case in which `packed(2)` actually lowers the element
// alignment rather than merely leaving it unchanged.
util_assert_impl_all!(ReprCHomogeneousPacked2Sized<ReprCAligned8, 0>: imp::IntoBytes);
util_assert_impl_all!(ReprCHomogeneousPacked2Dst<ReprCAligned8, 3>: imp::IntoBytes);

// Zero-sized element types still carry alignment. In particular, a derive
// must not assume that an aligned field occupies any bytes.
util_assert_impl_all!(ReprCHomogeneousArrayThenSlice<ReprCAlignedZst, 0>: imp::IntoBytes);

// `IntoBytes` does not imply `Immutable`; interior-mutable element types are
// therefore part of the supported set too.
util_assert_impl_all!(
    ReprCHomogeneousArrayThenSlice<imp::UnsafeCell<util::AU16>, 3>: imp::IntoBytes
);

// A zero-length array does not eliminate the `T: IntoBytes` requirement: the
// trailing slice can still contain elements. `MaybeUninit<u8>` implements
// `Unaligned` but not `IntoBytes`, isolating the bound under test.
util_assert_not_impl_any!(
    ReprCHomogeneousArrayThenSlice<imp::MaybeUninit<u8>, 0>: imp::IntoBytes
);

// Generic `repr(C)` structs outside of the homogeneous family fall back to
// requiring every field type to be `Unaligned`.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCIndependentFields<T, U: ?imp::Sized> {
    t: T,
    u: U,
}

util_assert_impl_all!(ReprCIndependentFields<u8, [u8; 2]>: imp::IntoBytes);
util_assert_impl_all!(ReprCIndependentFields<u8, [[u8; 2]]>: imp::IntoBytes);
util_assert_not_impl_any!(ReprCIndependentFields<u8, util::AU16>: imp::IntoBytes);
util_assert_not_impl_any!(ReprCIndependentFields<u8, [util::AU16]>: imp::IntoBytes);

// The common element type is essential. Here, the zero-length `AU16` array
// gives the struct alignment 2, while an odd-length `[u8]` tail requires a
// trailing padding byte.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct ReprCHeterogeneousArrayThenSlice<T, U, const N: usize>([T; N], [U]);

util_assert_not_impl_any!(
    ReprCHeterogeneousArrayThenSlice<util::AU16, u8, 0>: imp::IntoBytes
);

//
// `repr(transparent)`
//

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct Transparent {
    a: u8,
    b: (),
}

util_assert_impl_all!(Transparent: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct TransparentGeneric<T: ?imp::Sized> {
    a: (),
    b: T,
}

util_assert_impl_all!(TransparentGeneric<u64>: imp::IntoBytes);
util_assert_impl_all!(TransparentGeneric<[u64]>: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct Unsized {
    a: [u8],
}

util_assert_impl_all!(Unsized: imp::IntoBytes);

// Deriving `IntoBytes` should work if the struct has bounded parameters.

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct WithParams<'a: 'b, 'b: 'a, T: 'a + 'b + imp::IntoBytes, const N: usize>(
    [T; N],
    imp::PhantomData<&'a &'b ()>,
)
where
    'a: 'b,
    'b: 'a,
    T: 'a + 'b + imp::IntoBytes;

util_assert_impl_all!(WithParams<'static, 'static, u8, 42>: imp::IntoBytes);

//
// Packed representations
//

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
struct CZstPacked;

util_assert_impl_all!(CZstPacked: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
struct CPacked {
    a: u8,
    // NOTE: The `u16` type is not guaranteed to have alignment 2, although it
    // does on many platforms. However, to fix this would require a custom type
    // with a `#[repr(align(2))]` attribute, and `#[repr(packed)]` types are not
    // allowed to transitively contain `#[repr(align(...))]` types. Thus, we
    // have no choice but to use `u16` here. Luckily, these tests run in CI on
    // platforms on which `u16` has alignment 2, so this isn't that big of a
    // deal.
    b: u16,
}

util_assert_impl_all!(CPacked: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed(2))]
// The same caveats as for CPacked apply - we're assuming u64 is at least
// 4-byte aligned by default. Without packed(2), this should fail, as there
// would be padding between a/b assuming u64 is 4+ byte aligned.
struct CPacked2 {
    a: u16,
    b: u64,
}

util_assert_impl_all!(CPacked2: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
struct CPackedGeneric<T, U: ?imp::Sized> {
    t: T,
    // Unsized types stored in `repr(packed)` structs must not be dropped
    // because dropping them in-place might be unsound depending on the
    // alignment of the outer struct. Sized types can be dropped by first being
    // moved to an aligned stack variable, but this isn't possible with unsized
    // types.
    u: imp::ManuallyDrop<U>,
}

util_assert_impl_all!(CPackedGeneric<u8, util::AU16>: imp::IntoBytes);
util_assert_impl_all!(CPackedGeneric<u8, [util::AU16]>: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(packed)]
struct PackedGeneric<T, U: ?imp::Sized> {
    t: T,
    // Unsized types stored in `repr(packed)` structs must not be dropped
    // because dropping them in-place might be unsound depending on the
    // alignment of the outer struct. Sized types can be dropped by first being
    // moved to an aligned stack variable, but this isn't possible with unsized
    // types.
    u: imp::ManuallyDrop<U>,
}

util_assert_impl_all!(PackedGeneric<u8, util::AU16>: imp::IntoBytes);
util_assert_impl_all!(PackedGeneric<u8, [util::AU16]>: imp::IntoBytes);

// Test for the failure reported in #1182.

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
pub struct IndexEntryFlags(u8);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
pub struct IndexEntry<const SIZE_BLOCK_ID: usize> {
    block_number: imp::native_endian::U64,
    flags: IndexEntryFlags,
    block_id: [u8; SIZE_BLOCK_ID],
}

util_assert_impl_all!(IndexEntry<0>: imp::IntoBytes);
util_assert_impl_all!(IndexEntry<1>: imp::IntoBytes);

//
// Rust representation
//

// This test is non-portable, but works so long as Rust happens to lay this
// struct out with no padding.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Unpacked {
    a: u8,
    b: u8,
}

util_assert_impl_all!(Unpacked: imp::IntoBytes);
