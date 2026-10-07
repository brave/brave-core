/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

/// This source provides extra C++ bindings which are used to handle 64bit
/// integers over base::Value types.
#[cxx::bridge(namespace=serde_json_lenient)]
mod ffi {
    unsafe extern "C++" {
        include!("brave/chromium_src/third_party/rust/serde_json_lenient/v0_2/wrapper/functions.h");

        type Dict = crate::Dict;
        type List = crate::List;

        fn list_append_i64(ctx: Pin<&mut List>, val: i64);
        fn list_append_u64(ctx: Pin<&mut List>, val: u64);
        fn dict_set_i64(ctx: Pin<&mut Dict>, key: &str, val: i64);
        fn dict_set_u64(ctx: Pin<&mut Dict>, key: &str, val: u64);
    }
}

pub use ffi::*;

use crate::visitor::DeserializationTarget;

/// The customisation point to provide custom handling for i64 base::Value.
pub fn handle_large_i64<E: serde::de::Error>(
    aggregate: DeserializationTarget,
    value: i64,
) -> Result<(), E> {
    {
        match aggregate {
            DeserializationTarget::List { ctx } => list_append_i64(ctx, value),
            DeserializationTarget::Dict { ctx, key } => dict_set_i64(ctx, key, value),
        }
        Ok(())
    }
}

/// The customisation point to provide custom handling for u64 base::Value.
pub fn handle_large_u64<E: serde::de::Error>(
    aggregate: DeserializationTarget,
    value: u64,
) -> Result<(), E> {
    {
        match aggregate {
            DeserializationTarget::List { ctx } => list_append_u64(ctx, value),
            DeserializationTarget::Dict { ctx, key } => dict_set_u64(ctx, key, value),
        }
        Ok(())
    }
}
