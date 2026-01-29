/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use candle_embedder::Gemma3Embedder;
use libc::{c_char, c_void, size_t};
use std::ffi::CStr;
use std::panic;
use std::slice;

#[repr(C)]
pub struct CCandleEmbedder {
    embedder: Box<Gemma3Embedder>,
}

pub type InitCallback = extern "C" fn(*mut c_void, bool, *const c_char);
pub type EmbedCallback = extern "C" fn(*mut c_void, *const f32, size_t);

#[no_mangle]
pub extern "C" fn candle_embedder_new(
    weights: *const u8,
    weights_len: size_t,
    weights_dense1: *const u8,
    weights_dense1_len: size_t,
    weights_dense2: *const u8,
    weights_dense2_len: size_t,
    tokenizer: *const u8,
    tokenizer_len: size_t,
    config: *const u8,
    config_len: size_t,
    init_cb: InitCallback,
    user_data: *mut c_void,
) {
    let result = panic::catch_unwind(|| {
        let weights_slice = unsafe { slice::from_raw_parts(weights, weights_len) };
        let weights_dense1_slice =
            unsafe { slice::from_raw_parts(weights_dense1, weights_dense1_len) };
        let weights_dense2_slice =
            unsafe { slice::from_raw_parts(weights_dense2, weights_dense2_len) };
        let tokenizer_slice = unsafe { slice::from_raw_parts(tokenizer, tokenizer_len) };
        let config_slice = unsafe { slice::from_raw_parts(config, config_len) };

        Gemma3Embedder::new(
            weights_slice.to_vec(),
            weights_dense1_slice.to_vec(),
            weights_dense2_slice.to_vec(),
            tokenizer_slice.to_vec(),
            config_slice.to_vec(),
        )
    });

    match result {
        Ok(Ok(embedder)) => {
            let boxed = Box::new(CCandleEmbedder { embedder: Box::new(embedder) });
            let ptr = Box::into_raw(boxed) as *mut c_void;
            init_cb(user_data, true, ptr as *const c_char);
        }
        Ok(Err(e)) => {
            let error_msg = format!("Initialization failed: {}", e);
            let c_error = std::ffi::CString::new(error_msg).unwrap();
            init_cb(user_data, false, c_error.as_ptr());
        }
        Err(_) => {
            let c_error = std::ffi::CString::new("Panic during initialization").unwrap();
            init_cb(user_data, false, c_error.as_ptr());
        }
    }
}

#[no_mangle]
pub extern "C" fn candle_embedder_embed(
    embedder: *mut CCandleEmbedder,
    text: *const c_char,
    embed_cb: EmbedCallback,
    user_data: *mut c_void,
) {
    let result = panic::catch_unwind(|| {
        let embedder = unsafe { &*embedder };
        let text_cstr = unsafe { CStr::from_ptr(text) };
        let text_str = text_cstr.to_str().unwrap();

        embedder.embedder.embed(text_str)
    });

    match result {
        Ok(Ok(embedding)) => {
            embed_cb(user_data, embedding.as_ptr(), embedding.len());
        }
        Ok(Err(_)) | Err(_) => {
            embed_cb(user_data, std::ptr::null(), 0);
        }
    }
}

#[no_mangle]
pub extern "C" fn candle_embedder_destroy(embedder: *mut CCandleEmbedder) {
    if !embedder.is_null() {
        let _ = panic::catch_unwind(|| unsafe {
            let _ = Box::from_raw(embedder);
        });
    }
}
