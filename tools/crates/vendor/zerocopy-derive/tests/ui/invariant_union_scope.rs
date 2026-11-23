// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

extern crate zerocopy_renamed;

use zerocopy_renamed::TryFromBytes;

// Keep name-resolution errors separate from macro-expansion errors, which
// can prevent rustc from reporting these diagnostics.
#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
union Union {
    #[zerocopy(invariant(**b.unaligned_as_ref() > 0))]
    //~[msrv, stable, nightly]^ ERROR: cannot find value `b` in this scope
    a: u8,
    #[zerocopy(invariant(**a.unaligned_as_ref() > 0))]
    //~[msrv, stable, nightly]^ ERROR: cannot find value `a` in this scope
    b: u8,
}

fn main() {}
