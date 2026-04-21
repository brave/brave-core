// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

//! Empty stub. This crate exists solely so Cargo accepts the manifest at
//! ../Cargo.toml as a valid package, which in turn lets us pin `boringtun`
//! as a git dependency in Cargo.lock.
//!
//! We never build this crate. The actual library shipped to consumers is
//! the `boringtun` cdylib produced by
//! `cargo build -p boringtun --release`. See `brave/script/build_boringtun.py`.
