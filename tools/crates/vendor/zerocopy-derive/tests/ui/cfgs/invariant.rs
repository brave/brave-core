// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

extern crate zerocopy_renamed;

use zerocopy_renamed::{FromBytes, TryFromBytes};

#[derive(TryFromBytes)]
//~^ ERROR: `invariant` is experimental; pass '--cfg zerocopy_unstable_ptr' to enable
#[zerocopy(crate = "zerocopy_renamed")]
struct Struct {
    #[zerocopy(invariant(true))]
    a: u8,
}

#[derive(TryFromBytes)]
//~^ ERROR: `invariant` is experimental; pass '--cfg zerocopy_unstable_ptr' to enable
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum Enum {
    A {
        #[zerocopy(invariant(true))]
        a: u8,
    },
}

// Skipping an unsupported derive must still require the experimental cfgs.
#[derive(FromBytes)]
//~^ ERROR: `on_error` is experimental; pass '--cfg zerocopy_unstable_linux' to enable
//~^^ ERROR: `invariant` is experimental; pass '--cfg zerocopy_unstable_ptr' to enable
#[zerocopy(crate = "zerocopy_renamed", on_error = "skip")]
struct Skipped {
    #[zerocopy(invariant(true))]
    a: u8,
}

#[derive(TryFromBytes)]
//~^ ERROR: `invariant` is experimental; pass '--cfg zerocopy_unstable_ptr' to enable
#[zerocopy(crate = "zerocopy_renamed")]
union Union {
    #[zerocopy(invariant(true))]
    a: u8,
}

#[derive(TryFromBytes)]
//~^ ERROR: `on_error` is experimental; pass '--cfg zerocopy_unstable_linux' to enable
//~^^ ERROR: `invariant` is experimental; pass '--cfg zerocopy_unstable_ptr' to enable
#[zerocopy(crate = "zerocopy_renamed", on_error = "skip")]
enum SkippedEnum {
    A {
        #[zerocopy(invariant(true))]
        a: u8,
    },
}

fn main() {}
