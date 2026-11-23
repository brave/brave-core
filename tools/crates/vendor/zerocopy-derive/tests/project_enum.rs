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

// In the `repr(C)` layout, the tag precedes a union of variant payloads. In the
// primitive layout, the tag is instead the first field of each variant payload.
// The differently-aligned fields below ensure that confusing those layouts
// would project to the wrong offsets.
#[derive(Clone, Copy, imp::KnownLayout, imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
enum ReprC {
    UnitLike,
    TupleLike(u8, u16),
    StructLike { a: u8, b: u16 },
}

#[derive(Clone, Copy, imp::KnownLayout, imp::Project)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum ReprU8 {
    UnitLike,
    TupleLike(u8, u16),
    StructLike { a: u8, b: u16 },
}

// Rustfmt repeatedly indents multiline generic method calls inside this macro.
// The false `cfg` hides its tool attribute from rustc while rustfmt honors it.
#[cfg_attr(any(), rustfmt::skip)]
macro_rules! test_enum {
    ($module:ident, $ty:ident) => {
        mod $module {
            use super::imp;

            type Enum = super::$ty;

            const TUPLE_BYTE: u8 = 0x12;
            const TUPLE_WORD: u16 = 0x3456;
            const STRUCT_BYTE: u8 = 0x78;
            const STRUCT_WORD: u16 = 0x9ABC;

            #[test]
            fn tag() {
                let read_tag = |value: &Enum| {
                    imp::Ptr::from_ref(value)
                        .project_tag::<imp::project_clients::ProjectDerive>()
                        .read::<imp::BecauseImmutable>()
                        as isize
                };

                let unit_tag = read_tag(&Enum::UnitLike);
                let tuple_tag = read_tag(&Enum::TupleLike(0, 0));
                let struct_tag = read_tag(&Enum::StructLike { a: 0, b: 0 });

                imp::assert_eq!(unit_tag, 0);
                imp::assert_eq!(tuple_tag, 1);
                imp::assert_eq!(struct_tag, 2);
            }

            #[test]
            fn uninit() {
                let mut storage = imp::MaybeUninit::<Enum>::uninit();
                let mut ptr = imp::Ptr::from_mut(&mut storage)
                    .transmute::<Enum, imp::invariant::Uninit, _>()
                    .try_into_aligned()
                    .unwrap();

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(0) },
                    >()
                    .unwrap()
                    .transmute::<
                        imp::MaybeUninit<u8>,
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .bikeshed_recall_aligned()
                    .as_mut()
                    .write(TUPLE_BYTE);
                imp::assert_eq!(*field, TUPLE_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(1) },
                    >()
                    .unwrap()
                    .transmute::<
                        imp::MaybeUninit<u16>,
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .try_into_aligned()
                    .unwrap()
                    .as_mut()
                    .write(TUPLE_WORD);
                imp::assert_eq!(*field, TUPLE_WORD);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(a) },
                    >()
                    .unwrap()
                    .transmute::<
                        imp::MaybeUninit<u8>,
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .bikeshed_recall_aligned()
                    .as_mut()
                    .write(STRUCT_BYTE);
                imp::assert_eq!(*field, STRUCT_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(b) },
                    >()
                    .unwrap()
                    .transmute::<
                        imp::MaybeUninit<u16>,
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .try_into_aligned()
                    .unwrap()
                    .as_mut()
                    .write(STRUCT_WORD);
                imp::assert_eq!(*field, STRUCT_WORD);

                // A `MaybeUninit<Enum>` is the strictest type through which all
                // bytes of an `Uninit` `Ptr<Enum>` may be read.
                let _: imp::MaybeUninit<Enum> = *ptr
                    .transmute::<
                        imp::MaybeUninit<Enum>,
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .try_into_aligned()
                    .unwrap()
                    .as_ref();
            }

            #[test]
            fn initialized() {
                const SIZE: usize = imp::core::mem::size_of::<Enum>();

                // The zero-length array gives the byte array `Enum`'s
                // alignment without contributing to the storage size.
                #[repr(C)]
                struct AlignedBytes([Enum; 0], [u8; SIZE]);

                let mut storage = AlignedBytes([], [0; SIZE]);
                let bytes = imp::Ptr::from_mut(&mut storage.1).as_slice();
                let mut ptr = bytes.try_cast_into_no_leftover::<Enum, _>(imp::None).unwrap();

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(0) },
                    >()
                    .unwrap()
                    .recall_validity::<
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .as_mut();
                *field = TUPLE_BYTE;
                imp::assert_eq!(*field, TUPLE_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(1) },
                    >()
                    .unwrap()
                    .recall_validity::<
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .as_mut();
                *field = TUPLE_WORD;
                imp::assert_eq!(*field, TUPLE_WORD);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(a) },
                    >()
                    .unwrap()
                    .recall_validity::<
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .as_mut();
                *field = STRUCT_BYTE;
                imp::assert_eq!(*field, STRUCT_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(b) },
                    >()
                    .unwrap()
                    .recall_validity::<
                        imp::invariant::Safe,
                        (_, (_, imp::BecauseExclusive)),
                    >()
                    .as_mut();
                *field = STRUCT_WORD;
                imp::assert_eq!(*field, STRUCT_WORD);

                // Initialized bytes are valid `u8`s, so a byte slice is the
                // strictest form through which the entire pointer may be read.
                let bytes = ptr.as_bytes().as_ref();
                imp::assert_eq!(bytes.len(), SIZE);
                imp::assert_ne!(bytes, &[0; SIZE]);
            }

            #[test]
            fn valid() {
                let mut value = Enum::UnitLike;
                let mut ptr = imp::Ptr::from_mut(&mut value);
                let wrong_variant = ptr.reborrow().project::<
                    imp::project_clients::ProjectDerive,
                    _,
                    { imp::ident_id!(TupleLike) },
                    { imp::ident_id!(0) },
                >();
                imp::assert!(wrong_variant.is_err());
                imp::assert_eq!(
                    imp::core::mem::discriminant(ptr.as_ref()),
                    imp::core::mem::discriminant(&Enum::UnitLike),
                );

                let mut value = Enum::TupleLike(0, 0);
                let mut ptr = imp::Ptr::from_mut(&mut value);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(0) },
                    >()
                    .unwrap()
                    .as_mut();
                *field = TUPLE_BYTE;
                imp::assert_eq!(*field, TUPLE_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(TupleLike) },
                        { imp::ident_id!(1) },
                    >()
                    .unwrap()
                    .as_mut();
                *field = TUPLE_WORD;
                imp::assert_eq!(*field, TUPLE_WORD);

                let wrong_variant = ptr.reborrow().project::<
                    imp::project_clients::ProjectDerive,
                    _,
                    { imp::ident_id!(StructLike) },
                    { imp::ident_id!(a) },
                >();
                imp::assert!(wrong_variant.is_err());

                match ptr.as_ref() {
                    Enum::TupleLike(a, b) => {
                        imp::assert_eq!(*a, TUPLE_BYTE);
                        imp::assert_eq!(*b, TUPLE_WORD);
                    }
                    _ => imp::assert!(false),
                }

                let mut value = Enum::StructLike { a: 0, b: 0 };
                let mut ptr = imp::Ptr::from_mut(&mut value);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(a) },
                    >()
                    .unwrap()
                    .as_mut();
                *field = STRUCT_BYTE;
                imp::assert_eq!(*field, STRUCT_BYTE);

                let field = ptr
                    .reborrow()
                    .project::<
                        imp::project_clients::ProjectDerive,
                        _,
                        { imp::ident_id!(StructLike) },
                        { imp::ident_id!(b) },
                    >()
                    .unwrap()
                    .as_mut();
                *field = STRUCT_WORD;
                imp::assert_eq!(*field, STRUCT_WORD);

                let wrong_variant = ptr.reborrow().project::<
                    imp::project_clients::ProjectDerive,
                    _,
                    { imp::ident_id!(TupleLike) },
                    { imp::ident_id!(0) },
                >();
                imp::assert!(wrong_variant.is_err());

                match ptr.as_ref() {
                    Enum::StructLike { a, b } => {
                        imp::assert_eq!(*a, STRUCT_BYTE);
                        imp::assert_eq!(*b, STRUCT_WORD);
                    }
                    _ => imp::assert!(false),
                }
            }
        }
    };
}

test_enum!(repr_c, ReprC);
test_enum!(repr_u8, ReprU8);
