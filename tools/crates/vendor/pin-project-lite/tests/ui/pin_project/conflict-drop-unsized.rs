// SPDX-License-Identifier: Apache-2.0 OR MIT

use pin_project_lite::pin_project;

pin_project! { //~ ERROR E0119
    struct TraitObject {
        #[pin]
        inner: dyn std::fmt::Debug,
    }
}

impl Drop for TraitObject {
    fn drop(&mut self) {}
}

pin_project! { //~ ERROR E0119
    struct Slice {
        #[pin]
        inner: [u8],
    }
}

impl Drop for Slice {
    fn drop(&mut self) {}
}

fn main() {}
