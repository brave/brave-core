// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::str::FromStr;

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = filecoin)]
mod ffi {
    extern "Rust" {
        fn is_valid_cid(cid: &str) -> bool;
        fn ensure_cidv1(cid_str: &str) -> String;
    }
}

pub fn is_valid_cid(cid_str: &str) -> bool {
    return cid::Cid::from_str(cid_str).is_ok();
}

fn ensure_cidv1(cid_str: &str) -> String {
    let cid = match cid_str.parse::<cid::Cid>() {
        Ok(parsed) => parsed,
        Err(_) => return String::new(),
    };
    
    if cid.version() == cid::Version::V1 {
        cid_str.to_string()
    } else {
        cid.into_v1()
            .map(|cid_v1| cid_v1.to_string())
            .unwrap_or_else(|_| String::new())
    }
}
