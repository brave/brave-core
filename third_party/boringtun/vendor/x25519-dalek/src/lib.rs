// -*- mode: rust; -*-
//
// This file is part of x25519-dalek.
// Copyright (c) 2017-2021 isis lovecruft
// Copyright (c) 2019-2021 DebugSteven
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>
// - DebugSteven <debugsteven@gmail.com>

// Refuse to compile if documentation is missing, but only on nightly.
//
// This means that missing docs will still fail CI, but means we can use
// README.md as the crate documentation.

#![no_std]
#![cfg_attr(feature = "bench", feature(test))]
#![cfg_attr(docsrs, feature(doc_auto_cfg, doc_cfg, doc_cfg_hide))]
#![cfg_attr(docsrs, doc(cfg_hide(docsrs)))]
#![deny(missing_docs)]
#![doc(
    html_logo_url = "https://cdn.jsdelivr.net/gh/dalek-cryptography/curve25519-dalek/docs/assets/dalek-logo-clear.png"
)]
#![doc = include_str!("../README.md")]

//------------------------------------------------------------------------
// x25519-dalek public API
//------------------------------------------------------------------------

mod x25519;

pub use crate::x25519::*;
