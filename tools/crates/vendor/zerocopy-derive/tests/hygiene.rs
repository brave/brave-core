// Copyright 2019 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

// Make sure that macro hygiene will ensure that when we reference "zerocopy",
// that will work properly even if they've renamed the crate and have not
// imported its traits.

// See comment in `include.rs` for why we disable the prelude.
#![no_implicit_prelude]
#![allow(warnings)]

include!("include.rs");

extern crate zerocopy_renamed as _zerocopy;

extern crate self as derive_path_test;

pub mod nested {
    pub extern crate zerocopy_renamed as reexported_zerocopy;
}

macro_rules! test {
    ($($path:tt)::*, $crate_str:tt) => {
        #[derive(
            $($path)::*::KnownLayout,
            $($path)::*::FromBytes,
            $($path)::*::Unaligned,
        )]
        #[zerocopy(crate = $crate_str)]
        #[repr(C)]
        struct TypeParams<'a, T, I: imp::Iterator> {
            a: T,
            c: I::Item,
            d: u8,
            e: imp::PhantomData<&'a [::core::primitive::u8]>,
            f: imp::PhantomData<&'static ::core::primitive::str>,
            g: imp::PhantomData<imp::String>,
        }

        util_assert_impl_all!(
            TypeParams<'static, (), imp::IntoIter<()>>:
                $($path)::*::KnownLayout,
                $($path)::*::FromZeros,
                $($path)::*::FromBytes,
                $($path)::*::Unaligned
        );
    };
}

test!(_zerocopy, "zerocopy_renamed");

mod nested_external {
    use super::*;

    test!(
        derive_path_test::nested::reexported_zerocopy,
        "derive_path_test::nested::reexported_zerocopy"
    );
}

mod crate_relative {
    use super::*;

    test!(crate::nested::reexported_zerocopy, "crate::nested::reexported_zerocopy");
}

mod super_relative {
    use super::*;

    test!(super::nested::reexported_zerocopy, "super::nested::reexported_zerocopy");
}

mod self_super_relative {
    use super::*;

    test!(self::super::nested::reexported_zerocopy, "self::super::nested::reexported_zerocopy");
}

mod super_super_relative {
    mod inner {
        use super::super::*;

        test!(
            super::super::nested::reexported_zerocopy,
            "super::super::nested::reexported_zerocopy"
        );
    }
}

// Regression test for #2177.
//
// This test ensures that `#[derive(KnownLayout)]` does not trigger the
// `private_bounds` lint when used on a public struct in a macro.
mod issue_2177 {
    #![deny(private_bounds)]
    // We need to access `_zerocopy` from the parent module.
    use super::_zerocopy;

    macro_rules! define {
        ($name:ident, $repr:ty) => {
            #[derive(_zerocopy::KnownLayout)]
            #[zerocopy(crate = "zerocopy_renamed")]
            #[repr(C)]
            pub struct $name($repr);
        };
    }

    define!(Foo, u8);
}

// Regression test for #3621. An inherent method on a concrete trailing field
// must not be selected in place of `KnownLayout::pointer_to_metadata`.
mod issue_3621 {
    use super::_zerocopy;

    #[derive(_zerocopy::KnownLayout)]
    #[zerocopy(crate = "zerocopy_renamed")]
    #[repr(C)]
    struct Inner([u8]);

    impl Inner {
        fn pointer_to_metadata() {}
    }

    #[derive(_zerocopy::KnownLayout)]
    #[zerocopy(crate = "zerocopy_renamed")]
    #[repr(C)]
    struct Outer(Inner);
}

// Field names that match constants, unit structs, or const parameters in the
// surrounding scope must not interfere with validation.
mod field_bindings {
    use super::{imp, util};

    const CONST: () = ();
    const candidate_: () = ();
    struct UNIT;

    #[derive(imp::TryFromBytes)]
    #[zerocopy(crate = "zerocopy_renamed")]
    #[repr(C)]
    struct Struct<const N: usize> {
        CONST: bool,
        UNIT: bool,
        N: bool,
    }

    #[derive(imp::TryFromBytes)]
    #[zerocopy(crate = "zerocopy_renamed")]
    #[repr(u8)]
    enum Enum<const N: usize> {
        Named { CONST: bool, UNIT: bool, N: bool },
    }

    #[derive(imp::TryFromBytes)]
    #[zerocopy(crate = "zerocopy_renamed")]
    union Union<const N: usize> {
        CONST: bool,
        UNIT: bool,
        N: bool,
    }

    #[test]
    fn test_field_bindings() {
        util::test_is_safe::<Struct<0>, _>([0u8; 3], true);
        util::test_is_safe::<Enum<0>, _>([0u8; 4], true);
        for idx in 0..3 {
            let mut fields = [0u8; 3];
            fields[idx] = 2;
            util::test_is_safe::<Struct<0>, _>(fields, false);
            util::test_is_safe::<Enum<0>, _>([0u8, fields[0], fields[1], fields[2]], false);
        }
        util::test_is_safe::<Union<0>, _>([0u8], true);
        util::test_is_safe::<Union<0>, _>([1u8], true);
        util::test_is_safe::<Union<0>, _>([2u8], false);
    }
}
