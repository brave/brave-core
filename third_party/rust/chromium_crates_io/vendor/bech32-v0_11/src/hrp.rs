// SPDX-License-Identifier: MIT

//! Re-exports the hrp types from [`primitives::hrp`] to make importing ergonomic for the top level APIs.
//!
//! [`primitives::hrp`]: crate::primitives::hrp

#[doc(inline)]
pub use crate::primitives::hrp::{Hrp, BC, BCRT, TB};
