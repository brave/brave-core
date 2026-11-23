// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// You may not use this file except in compliance with those licenses.

// See comment in `include.rs` for why we disable the prelude.
#![no_implicit_prelude]
#![allow(warnings)]

include!("include.rs");

#[derive(Clone, Copy, imp::FromBytes, imp::IntoBytes, imp::KnownLayout)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(2))]
struct Align2([u8; 2]);

// `first` is followed by one byte of padding needed to align `second`, and
// `second` is followed by four bytes of trailing padding needed to align the
// struct.
#[derive(Clone, Copy, imp::KnownLayout, imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C, align(8))]
struct Padded {
    first: u8,
    second: Align2,
}

const FIRST: u8 = 0x12;
const SECOND: Align2 = Align2([0x34, 0x56]);

#[test]
fn uninit() {
    let mut storage = imp::MaybeUninit::<Padded>::uninit();
    let mut ptr = imp::Ptr::from_mut(&mut storage)
        .transmute::<Padded, imp::invariant::Uninit, _>()
        .try_into_aligned()
        .unwrap();

    let first = ptr
        .reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(first) },
        >()
        .unwrap()
        .transmute::<
            imp::MaybeUninit<u8>,
            imp::invariant::Safe,
            (_, (_, imp::BecauseExclusive)),
        >()
        .bikeshed_recall_aligned()
        .as_mut()
        .write(FIRST);
    imp::assert_eq!(*first, FIRST);

    let second = ptr
        .reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(second) },
        >()
        .unwrap()
        .transmute::<
            imp::MaybeUninit<Align2>,
            imp::invariant::Safe,
            (_, (_, imp::BecauseExclusive)),
        >()
        .try_into_aligned()
        .unwrap()
        .as_mut()
        .write(SECOND);
    imp::assert_eq!(second.0, SECOND.0);

    // A `MaybeUninit<Padded>` is the strictest type through which all bytes of
    // an `Uninit` `Ptr<Padded>` may be read.
    let _: imp::MaybeUninit<Padded> = *ptr
        .transmute::<
            imp::MaybeUninit<Padded>,
            imp::invariant::Safe,
            (_, (_, imp::BecauseExclusive)),
        >()
        .try_into_aligned()
        .unwrap()
        .as_ref();
}

#[test]
fn initialized() {
    #[repr(C, align(8))]
    struct AlignedBytes([u8; 8]);

    let mut storage = AlignedBytes([0; 8]);
    let bytes = imp::Ptr::from_mut(&mut storage.0).as_slice();
    let mut ptr = bytes.try_cast_into_no_leftover::<Padded, _>(imp::None).unwrap();

    let first = ptr
        .reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(first) },
        >()
        .unwrap()
        .recall_validity::<imp::invariant::Safe, (_, (_, imp::BecauseExclusive))>();
    *first.as_mut() = FIRST;
    imp::assert_eq!(
        *ptr.reborrow()
            .project::<
                imp::project_clients::ProjectDerive,
                _,
                { imp::STRUCT_VARIANT_ID },
                { imp::ident_id!(first) },
            >()
            .unwrap()
            .recall_validity::<imp::invariant::Safe, (_, (_, imp::BecauseExclusive))>()
            .as_ref(),
        FIRST,
    );

    let second = ptr
        .reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(second) },
        >()
        .unwrap()
        .recall_validity::<imp::invariant::Safe, (_, (_, imp::BecauseExclusive))>();
    *second.as_mut() = SECOND;
    imp::assert_eq!(
        ptr.reborrow()
            .project::<
                imp::project_clients::ProjectDerive,
                _,
                { imp::STRUCT_VARIANT_ID },
                { imp::ident_id!(second) },
            >()
            .unwrap()
            .recall_validity::<imp::invariant::Safe, (_, (_, imp::BecauseExclusive))>()
            .as_ref()
            .0,
        SECOND.0,
    );

    // Initialized bytes are valid `u8`s, so a byte slice is the strictest
    // form through which the entire `Ptr<Padded>` may be read.
    let bytes = ptr.as_bytes().as_ref();
    imp::assert_eq!(bytes, &[FIRST, 0, 0x34, 0x56, 0, 0, 0, 0]);
}

#[test]
fn valid() {
    let mut value = Padded { first: 0, second: Align2([0; 2]) };
    let mut ptr = imp::Ptr::from_mut(&mut value);

    *ptr.reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(first) },
        >()
        .unwrap()
        .as_mut() = FIRST;
    imp::assert_eq!(
        *ptr.reborrow()
            .project::<
                imp::project_clients::ProjectDerive,
                _,
                { imp::STRUCT_VARIANT_ID },
                { imp::ident_id!(first) },
            >()
            .unwrap()
            .as_ref(),
        FIRST,
    );

    *ptr.reborrow()
        .project::<
            imp::project_clients::ProjectDerive,
            _,
            { imp::STRUCT_VARIANT_ID },
            { imp::ident_id!(second) },
        >()
        .unwrap()
        .as_mut() = SECOND;
    imp::assert_eq!(
        ptr.reborrow()
            .project::<
                imp::project_clients::ProjectDerive,
                _,
                { imp::STRUCT_VARIANT_ID },
                { imp::ident_id!(second) },
            >()
            .unwrap()
            .as_ref()
            .0,
        SECOND.0,
    );

    // A valid `Padded` is the strictest form through which a `Safe`
    // `Ptr<Padded>` may be read.
    let value = ptr.as_ref();
    imp::assert_eq!(value.first, FIRST);
    imp::assert_eq!(value.second.0, SECOND.0);
}
