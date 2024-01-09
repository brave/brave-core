// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = ipfs)]
mod ffi {
    extern "Rust" {
        fn is_car_valid(path: &str) -> bool;
    }
}

pub fn is_car_valid(path_str: &str) -> bool {    
    return path_str.is_empty();
}