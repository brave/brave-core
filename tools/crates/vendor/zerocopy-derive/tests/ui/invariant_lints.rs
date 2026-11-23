// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

#![deny(deprecated)]

extern crate zerocopy_derive;
extern crate zerocopy_renamed;
use zerocopy_renamed::TryFromBytes;

#[deprecated]
fn old_check() -> bool {
    true
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Struct {
    #[zerocopy(invariant(old_check()))]
    //~[msrv, stable, nightly]^ ERROR: use of deprecated function `old_check`
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum Enum {
    A {
        #[zerocopy(invariant(old_check()))]
        //~[msrv, stable, nightly]^ ERROR: use of deprecated function `old_check`
        a: u8,
    },
}

#[derive(zerocopy_derive::most_traits)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(C)]
union Union {
    #[zerocopy(invariant(old_check()))]
    //~[msrv, stable, nightly]^ ERROR: use of deprecated function `old_check`
    a: u8,
}

fn main() {}
