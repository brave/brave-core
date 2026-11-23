// Copyright 2019 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

use testutil::UiTestRunner;

#[test]
#[cfg_attr(
    any(
        miri,
        coverage_nightly,
        not(any(
            __ZEROCOPY_INTERNAL_USE_ONLY_TOOLCHAIN = "msrv",
            __ZEROCOPY_INTERNAL_USE_ONLY_TOOLCHAIN = "stable",
            __ZEROCOPY_INTERNAL_USE_ONLY_TOOLCHAIN = "nightly",
        )),
    ),
    ignore
)]
fn ui() {
    // This tests the behavior when experimental features are enabled.
    UiTestRunner::new()
        .rustc_arg("--cfg=zerocopy_derive_union_into_bytes")
        .rustc_arg("--cfg=zerocopy_unstable_linux")
        .rustc_arg("--cfg=zerocopy_unstable_ptr")
        .rustc_arg("-Wwarnings") // To ensure .stderr files reflect typical user encounter
        .run();

    // This tests the behavior when various `--cfg` flags are not present.
    UiTestRunner::new()
        .subdir("cfgs")
        .rustc_arg("-Wwarnings") // To ensure .stderr files reflect typical user encounter
        .run();
}
