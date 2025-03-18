// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

pub mod message;
pub mod signature;
use crate::signature::transaction_sign;
use std::str::FromStr;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = filecoin)]
mod ffi {
    extern "Rust" {
        fn transaction_sign(is_mainnet: bool, transaction: &str, private_key: &[u8]) -> String;
        fn is_valid_cid(cid: &str) -> bool;
    }
}

pub fn is_valid_cid(cid_str: &str) -> bool {
    return cid::Cid::from_str(cid_str).is_ok();
}
