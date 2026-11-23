// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://opensource.org/licenses/MIT>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

// See comment in `include.rs` for why we disable the prelude.
#![no_implicit_prelude]
#![allow(warnings)]

include!("include.rs");

type SharedAligned<V> = (imp::invariant::Shared, imp::invariant::Aligned, V);
type SharedUnaligned<V> = (imp::invariant::Shared, imp::invariant::Unaligned, V);

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Named {
    byte: u8,
    word: u16,
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Tuple(u8, u16);

#[test]
fn struct_fields() {
    let named = Named { byte: 1, word: 0x0203 };
    let word: imp::core::result::Result<
        imp::Ptr<'_, u16, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&named)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(word) }>();
    imp::assert_eq!(*word.unwrap().as_ref(), 0x0203);

    let tuple = Tuple(4, 0x0506);
    let field: imp::core::result::Result<
        imp::Ptr<'_, u16, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&tuple)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(1) }>();
    imp::assert_eq!(*field.unwrap().as_ref(), 0x0506);
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct NoPadding {
    first: u8,
    second: u8,
}

#[test]
fn struct_validity_is_preserved() {
    let value = NoPadding { first: 7, second: 8 };

    // SAFETY: `Uninit` permits every bit pattern.
    let uninit = unsafe { imp::Ptr::from_ref(&value).assume_validity::<imp::invariant::Uninit>() };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Uninit>>,
        imp::core::convert::Infallible,
    > = uninit
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(second) }>();
    imp::assert!(projected.is_ok());

    // SAFETY: `NoPadding` consists of two `u8` fields and has no padding, so
    // all of its bytes are initialized.
    let initialized = unsafe { imp::Ptr::from_ref(&value).assume_initialized() };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Initialized>>,
        imp::core::convert::Infallible,
    > = initialized
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(second) }>();
    imp::assert!(projected.is_ok());

    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(second) }>();
    imp::assert_eq!(*projected.unwrap().as_ref(), 8);
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, packed)]
struct Packed {
    byte: u8,
    word: u32,
}

#[test]
fn packed_struct_loses_alignment() {
    let value = Packed { byte: 1, word: 0x0203_0405 };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u32, SharedUnaligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(word) }>();
    imp::assert_eq!(projected.unwrap().read::<imp::BecauseImmutable>(), 0x0203_0405);
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct Unsized<T: ?imp::Sized> {
    head: u8,
    tail: T,
}

fn project_unsized_tail(
    value: imp::Ptr<'_, Unsized<[u8]>, SharedAligned<imp::invariant::Safe>>,
) -> imp::core::result::Result<
    imp::Ptr<'_, [u8], SharedAligned<imp::invariant::Safe>>,
    imp::core::convert::Infallible,
> {
    value.project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(tail) }>()
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
union Union {
    unsigned: u32,
    signed: i32,
}

#[test]
fn union_uninit_and_initialized() {
    let value = Union { unsigned: 0x0102_0304 };

    // SAFETY: `Uninit` permits every bit pattern.
    let uninit = unsafe { imp::Ptr::from_ref(&value).assume_validity::<imp::invariant::Uninit>() };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, i32, SharedAligned<imp::invariant::Uninit>>,
        imp::core::convert::Infallible,
    > = uninit
        .project::<imp::project_clients::ProjectDerive, _, { imp::REPR_C_UNION_VARIANT_ID }, { imp::ident_id!(signed) }>();
    imp::assert!(projected.is_ok());

    // SAFETY: `value` was initialized through its `u32` field, which occupies
    // every byte of this union.
    let initialized = unsafe { imp::Ptr::from_ref(&value).assume_initialized() };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, i32, SharedAligned<imp::invariant::Initialized>>,
        imp::core::convert::Infallible,
    > = initialized
        .project::<imp::project_clients::ProjectDerive, _, { imp::REPR_C_UNION_VARIANT_ID }, { imp::ident_id!(signed) }>();
    imp::assert!(projected.is_ok());
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
enum Fieldless {
    A,
    B,
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
enum CEnum {
    Number(u32),
    Flag { value: u16 },
}

#[test]
fn repr_c_enum_checks_its_tag() {
    let value = CEnum::Flag { value: 0x1234 };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u16, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(Flag) }, { imp::ident_id!(value) }>();
    imp::assert_eq!(*projected.unwrap().as_ref(), 0x1234);

    let wrong_variant: imp::core::result::Result<
        imp::Ptr<'_, u32, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(Number) }, { imp::ident_id!(0) }>();
    imp::assert!(wrong_variant.is_err());
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum U8Enum {
    A(u8),
    B(u8),
}

#[test]
fn repr_u8_enum_validity_and_tag_checks() {
    let value = U8Enum::A(11);

    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(A) }, { imp::ident_id!(0) }>();
    imp::assert_eq!(*projected.unwrap().as_ref(), 11);

    let wrong_variant: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(B) }, { imp::ident_id!(0) }>();
    imp::assert!(wrong_variant.is_err());

    // SAFETY: `Uninit` permits every bit pattern. Unlike a `Safe` projection,
    // this projection must not inspect the tag.
    let uninit = unsafe { imp::Ptr::from_ref(&value).assume_validity::<imp::invariant::Uninit>() };
    let wrong_variant: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Uninit>>,
        imp::core::convert::Infallible,
    > = uninit.project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(B) }, { imp::ident_id!(0) }>();
    imp::assert!(wrong_variant.is_ok());

    imp::assert_eq!(imp::core::mem::size_of::<U8Enum>(), 2);
    // SAFETY: The primitive tag and the `A` variant's `u8` field occupy both
    // bytes of `value`, with no padding.
    let initialized = unsafe { imp::Ptr::from_ref(&value).assume_initialized() };
    let wrong_variant: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Initialized>>,
        imp::core::convert::Infallible,
    > = initialized
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(B) }, { imp::ident_id!(0) }>();
    imp::assert!(wrong_variant.is_ok());
}

// Projection must not add zerocopy trait bounds to field types.
#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Generic<T, U> {
    first: T,
    second: U,
}

#[test]
fn generic_fields_need_no_zerocopy_bounds() {
    let value = Generic { first: util::NotZerocopy(1u8), second: util::NotZerocopy(0x0203u16) };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, util::NotZerocopy<u16>, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(second) }>();
    imp::assert_eq!(projected.unwrap().as_ref().0, 0x0203);
}

#[derive(imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum GenericEnum<'a, T, const N: usize> {
    Borrowed(&'a T),
    Array([T; N]),
}

#[test]
fn generic_enum_fields_need_no_zerocopy_bounds() {
    let inner = util::NotZerocopy(0x1234u16);
    let value: GenericEnum<'_, util::NotZerocopy<u16>, 2> = GenericEnum::Borrowed(&inner);
    let projected: imp::core::result::Result<
        imp::Ptr<'_, &util::NotZerocopy<u16>, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(Borrowed) }, { imp::ident_id!(0) }>();
    imp::assert_eq!((**projected.unwrap().as_ref()).0, 0x1234);
}

mod visibility {
    #[derive(super::imp::Project)]
    #[zerocopy(crate = "zerocopy_renamed")]
    pub struct Public {
        pub field: u16,
        private: u8,
    }

    pub fn new() -> Public {
        Public { field: 0x1234, private: 0 }
    }
}

#[test]
fn public_marker_is_inferred_across_module_boundary() {
    let value = visibility::new();
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u16, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(field) }>();
    imp::assert_eq!(*projected.unwrap().as_ref(), 0x1234);
}

#[derive(imp::Project, imp::TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct ProjectThenTryFromBytes {
    field: u8,
}

#[derive(imp::TryFromBytes, imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
union TryFromBytesThenProject {
    field: u8,
}

#[derive(imp::Project, imp::TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum ProjectAndTryFromBytes {
    A(u8),
    B { field: bool },
}

fn assert_try_from_bytes<T: imp::TryFromBytes>() {}

#[test]
fn project_and_try_from_bytes_impls_coexist() {
    assert_try_from_bytes::<ProjectThenTryFromBytes>();
    assert_try_from_bytes::<TryFromBytesThenProject>();
    assert_try_from_bytes::<ProjectAndTryFromBytes>();

    let value = ProjectThenTryFromBytes { field: 42 };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Safe>>,
        imp::core::convert::Infallible,
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::STRUCT_VARIANT_ID }, { imp::ident_id!(field) }>();
    imp::assert_eq!(*projected.unwrap().as_ref(), 42);

    let value = TryFromBytesThenProject { field: 43 };
    // SAFETY: `Uninit` permits every bit pattern.
    let value = unsafe { imp::Ptr::from_ref(&value).assume_validity::<imp::invariant::Uninit>() };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, u8, SharedAligned<imp::invariant::Uninit>>,
        imp::core::convert::Infallible,
    > = value
        .project::<imp::project_clients::ProjectDerive, _, { imp::REPR_C_UNION_VARIANT_ID }, { imp::ident_id!(field) }>();
    imp::assert!(projected.is_ok());

    let value = ProjectAndTryFromBytes::B { field: true };
    let projected: imp::core::result::Result<
        imp::Ptr<'_, bool, SharedAligned<imp::invariant::Safe>>,
        (),
    > = imp::Ptr::from_ref(&value)
        .project::<imp::project_clients::ProjectDerive, _, { imp::ident_id!(B) }, { imp::ident_id!(field) }>();
    imp::assert!(*projected.unwrap().as_ref());
}
