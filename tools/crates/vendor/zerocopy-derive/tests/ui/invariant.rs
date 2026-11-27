// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

extern crate zerocopy_renamed;

use zerocopy_renamed::{FromBytes, FromZeros, TryFromBytes};

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Tuple(
    #[zerocopy(invariant(true))]
    //~[msrv, stable, nightly]^ ERROR: invariants are only supported on named fields
    u8,
);

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum TupleVariant {
    A(
        #[zerocopy(invariant(true))]
        //~[msrv, stable, nightly]^ ERROR: invariants are only supported on named fields
        u8,
    ),
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum VariantAttribute {
    #[zerocopy(invariant(true))]
    //~[msrv, stable, nightly]^ ERROR: invariants are only supported on named fields
    A,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct BareAttribute {
    #[zerocopy(invariant)]
    //~[msrv, stable, nightly]^ ERROR: expected `invariant(...)`
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct MissingExpression {
    #[zerocopy(invariant())]
    //~[msrv, stable, nightly]^ ERROR: unexpected end of input, expected an expression
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct ExtraExpression {
    #[zerocopy(invariant(true, false))]
    //~[msrv, stable, nightly]^ ERROR: unexpected token
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct MisspelledAttribute {
    #[zerocopy(invaraint(true))]
    //~[msrv, stable, nightly]^ ERROR: expected `invariant(...)`
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct NonBoolean {
    #[zerocopy(invariant(0u8))]
    //~[msrv, stable, nightly]^ ERROR: mismatched types
    a: u8,
}

#[derive(FromZeros)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Zeroable {
    #[zerocopy(invariant(**a.unaligned_as_ref() > 0))]
    //~[msrv, stable, nightly]^ ERROR: cannot derive `FromZeros` for a type with invariants
    a: u8,
}

#[derive(FromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Infallible {
    #[zerocopy(invariant(**a.unaligned_as_ref() > 0))]
    //~[msrv, stable, nightly]^ ERROR: cannot derive `FromBytes` for a type with invariants
    a: u8,
}

// Attribute errors must not be silently ignored by `on_error = "skip"`, or
// bypassed by the trivial `FromBytes` implementation of `is_safe`.
#[derive(FromBytes)]
#[zerocopy(crate = "zerocopy_renamed", on_error = "skip")]
struct SkippedTuple(
    #[zerocopy(invariant(true))]
    //~[msrv, stable, nightly]^ ERROR: invariants are only supported on named fields
    u8,
);

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Brackets {
    #[zerocopy(invariant[true])]
    //~[msrv, stable, nightly]^ ERROR: expected `invariant(...)`
    a: u8,
}

#[derive(TryFromBytes)]
#[zerocopy(crate = "zerocopy_renamed")]
struct Braces {
    #[zerocopy(invariant{true})]
    //~[msrv, stable, nightly]^ ERROR: expected `invariant(...)`
    a: u8,
}

fn main() {}
