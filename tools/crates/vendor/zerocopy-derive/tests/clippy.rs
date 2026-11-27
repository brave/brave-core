// Copyright 2026 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

#![deny(clippy::derive_partial_eq_without_eq)]

// Regression test for https://github.com/google/zerocopy/issues/3721. The
// `TryFromBytes` derive must not emit code that triggers
// `clippy::derive_partial_eq_without_eq` in downstream crates.
use zerocopy_renamed::{Immutable, TryFromBytes};

#[derive(TryFromBytes, Immutable)]
#[zerocopy(crate = "zerocopy_renamed")]
#[repr(u8)]
enum Kind {
    A = 1,
}

#[test]
fn generated_try_from_bytes_code_is_clippy_clean() {
    let _ = Kind::A;
}
