// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

//@[nightly,stable] check-pass

#![no_implicit_prelude]
#![deny(missing_debug_implementations)]
#![cfg_attr(msrv, deny(private_in_public))]

macro_rules! foo {
    () => {
        #[derive(::core::fmt::Debug, ::zerocopy_renamed::KnownLayout)]
        //~[msrv]^ ERROR: private type
        #[zerocopy(crate = "zerocopy_renamed")]
        #[repr(C)]
        pub struct Foo([::core::primitive::u8]);
    };
}

foo! {}
//~[msrv]^ ERROR: private type

fn main() {}
