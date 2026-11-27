// Copyright 2026 The Fuchsia Authors
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

// A generic `IntoBytes` type needs a recognized non-align representation.
// These definitions ensure that ordinary and raw `repr` spellings select the
// same transparent-layout proof, and separately exercise the raw spelling of
// the derive helper attribute's name.
#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct OrdinaryTransparent<T>(T);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[r#repr(r#transparent)]
struct RawTransparent<T>(T);

#[derive(imp::IntoBytes)]
#[r#zerocopy(crate = "zerocopy_renamed")]
#[repr(transparent)]
struct RawHelperTransparent<T>(T);

util_assert_impl_all!(OrdinaryTransparent<u8>: imp::IntoBytes);
util_assert_impl_all!(RawTransparent<u8>: imp::IntoBytes);
util_assert_impl_all!(RawHelperTransparent<u8>: imp::IntoBytes);

#[derive(imp::IntoBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
struct Wire([u8; 2]);

#[derive(imp::Immutable, imp::KnownLayout, imp::TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum OrdinaryPacket {
    Value(imp::core::num::NonZeroU8),
    End,
}

#[derive(imp::Immutable, imp::KnownLayout, imp::TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[r#repr(r#u8)]
enum RawPacket {
    Value(imp::core::num::NonZeroU8),
    End,
}

#[test]
fn test_enum_try_from_bytes() {
    // The `Value` payload is at byte offset one and is zero, so the candidate
    // is invalid. Both spellings must generate the same field validator.
    let bytes = [0, 0];
    util::test_is_safe::<OrdinaryPacket, _>(Wire(bytes), false);
    util::test_is_safe::<RawPacket, _>(Wire(bytes), false);
}

#[derive(imp::KnownLayout)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
#[repr(align(8))]
struct OrdinaryAligned(u8);

#[derive(imp::KnownLayout)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
#[r#repr(r#align(8))]
struct RawAligned(u8);

#[test]
fn test_known_layout() {
    let ordinary = <OrdinaryAligned as imp::KnownLayout>::size_for_metadata(());
    let raw = <RawAligned as imp::KnownLayout>::size_for_metadata(());
    let ordinary_size = imp::core::mem::size_of::<OrdinaryAligned>();
    let raw_size = imp::core::mem::size_of::<RawAligned>();
    imp::assert_eq!(ordinary_size, 8);
    imp::assert_eq!(raw_size, 8);
    imp::assert_eq!(ordinary, imp::Some(ordinary_size));
    imp::assert_eq!(raw, imp::Some(raw_size));
    imp::assert_eq!(ordinary, raw);
}
