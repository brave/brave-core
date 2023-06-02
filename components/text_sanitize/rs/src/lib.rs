// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#[cxx::bridge(namespace = text_sanitize)]
mod ffi {
    extern "Rust" {
        fn strip_non_alphanumeric_or_ascii_characters(text: &str) -> String;
    }
}

// Replaces all non-alphanumeric, non-ascii graphic, or non-ascii whitespace
// characters with whitespace.
fn strip_non_alphanumeric_or_ascii_characters(text: &str) -> String {
    text.chars()
        .map(|c| match c {
            x if x.is_alphanumeric() || x.is_ascii_graphic() || x.is_ascii_whitespace() => x,
            _ => ' ',
        })
        .collect()
}
