/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#![allow(dead_code)]

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

#[cfg(feature = "json_64bit_int_support")]
pub use ffi::*;

use crate::visitor::DeserializationTarget;

/// The customisation point to provide custom handling for i64 base::Value.
pub fn handle_large_i64<E: serde::de::Error>(
    _aggregate: DeserializationTarget,
    _value: i64,
) -> Result<(), E> {
    #[cfg(feature = "json_64bit_int_support")]
    {
        match _aggregate {
            DeserializationTarget::List { ctx } => list_append_i64(ctx, _value),
            DeserializationTarget::Dict { ctx, key } => dict_set_i64(ctx, key, _value),
        }
        Ok(())
    }
    #[cfg(not(feature = "json_64bit_int_support"))]
    {
        unreachable!(
            "handle_large_i64 should not be called without the 'json_64bit_int_support' feature"
        );
    }
}

/// The customisation point to provide custom handling for u64 base::Value.
pub fn handle_large_u64<E: serde::de::Error>(
    _aggregate: DeserializationTarget,
    _value: u64,
) -> Result<(), E> {
    #[cfg(feature = "json_64bit_int_support")]
    {
        match _aggregate {
            DeserializationTarget::List { ctx } => list_append_u64(ctx, _value),
            DeserializationTarget::Dict { ctx, key } => dict_set_u64(ctx, key, _value),
        }
        Ok(())
    }
    #[cfg(not(feature = "json_64bit_int_support"))]
    {
        unreachable!(
            "handle_large_u64 should not be called without the 'json_64bit_int_support' feature"
        );
    }
}
