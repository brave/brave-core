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

extern crate self as derive_path_test;

pub mod nested {
    pub extern crate zerocopy_renamed as reexported_zerocopy;
}

// Ensure that types that are use'd and types that are referenced by path work.

macro_rules! test {
    ($crate_path:tt) => {
        mod foo {
            use super::*;

            #[derive(imp::FromBytes, imp::IntoBytes, imp::Unaligned)]
            #[zerocopy(crate = $crate_path)]
            #[repr(C)]
            pub struct Foo {
                foo: u8,
            }

            #[derive(imp::FromBytes, imp::IntoBytes, imp::Unaligned)]
            #[zerocopy(crate = $crate_path)]
            #[repr(C)]
            pub struct Bar {
                bar: u8,
            }
        }

        use self::foo::Foo;

        #[derive(imp::FromBytes, imp::IntoBytes, imp::Unaligned)]
        #[zerocopy(crate = $crate_path)]
        #[repr(C)]
        struct Baz {
            foo: Foo,
            bar: foo::Bar,
        }
    };
}

test!("zerocopy_renamed");

mod root_relative {
    use super::*;

    test!("derive_path_test::nested::reexported_zerocopy");
}

mod crate_relative {
    use super::*;

    test!("crate::nested::reexported_zerocopy");
}

mod super_relative {
    use super::*;

    test!("super::nested::reexported_zerocopy");
}

mod self_super_relative {
    use super::*;

    test!("self::super::nested::reexported_zerocopy");
}

mod super_super_relative {
    use super::*;

    mod inner {
        use super::*;

        test!("super::super::nested::reexported_zerocopy");
    }
}
