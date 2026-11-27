// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

extern crate zerocopy_renamed;

use zerocopy_renamed::TryFromBytes;

const CONST: () = ();
struct Unit;

// Collisions must be rejected, not hidden by generated callable items.
#[derive(TryFromBytes)]
//~[msrv, stable, nightly]^ ERROR: let bindings cannot shadow constants
#[zerocopy(crate = "zerocopy_renamed")]
struct Constant {
    #[zerocopy(invariant(true))]
    CONST: u8,
}

#[derive(TryFromBytes)]
//~[msrv, stable, nightly]^ ERROR: let bindings cannot shadow unit structs
#[zerocopy(crate = "zerocopy_renamed")]
struct Constructor {
    #[zerocopy(invariant(true))]
    Unit: u8,
}

#[derive(TryFromBytes)]
//~[msrv, stable, nightly]^ ERROR: let bindings cannot shadow const parameters
#[zerocopy(crate = "zerocopy_renamed")]
struct ConstParam<const N: usize> {
    #[zerocopy(invariant(true))]
    N: u8,
}

fn main() {}
