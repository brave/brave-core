/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

fn main() {
    cxx_build::bridge("src/lib.rs").flag_if_supported("-std=c++14").compile("constellation-cxx");

    println!("cargo:rerun-if-changed=src/lib.rs");
}
