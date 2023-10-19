// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use super::*;
use libc::{c_void, strlen};
use std::panic::{self, AssertUnwindSafe};

// NOTE: we use `ExternOutputSink` proxy type, for extern handler function
struct ExternOutputSink {
    handler: unsafe extern "C" fn(*const c_char, size_t, *mut c_void),
    user_data: *mut c_void,
}

impl ExternOutputSink {
    #[inline]
    fn new(
        handler: unsafe extern "C" fn(*const c_char, size_t, *mut c_void),
        user_data: *mut c_void,
    ) -> Self {
        ExternOutputSink { handler, user_data }
    }
}

impl OutputSink for ExternOutputSink {
    #[inline]
    fn handle_chunk(&mut self, chunk: &[u8]) {
        let chunk_len = chunk.len();
        let chunk = chunk.as_ptr() as *const c_char;

        unsafe { (self.handler)(chunk, chunk_len, self.user_data) };
    }
}

/// Opaque structure to have the minimum amount of type safety across the FFI.
/// Only replaces c_void
#[repr(C)]
pub struct CRewriter {
    _private: [u8; 0],
}

/// New instance of SpeedReader. Must be freed by calling `speedreader_free`.
#[no_mangle]
pub extern "C" fn speedreader_new() -> *mut SpeedReader {
    to_ptr_mut(SpeedReader::default())
}

#[no_mangle]
pub extern "C" fn speedreader_free(speedreader: *mut SpeedReader) {
    assert_not_null!(speedreader);
    drop(to_box!(speedreader));
}

/// Returns SpeedReader rewriter instance for the given URL.
///
/// Returns NULL if no URL provided or initialization fails.
///
/// Results of rewriting are sent to `output_sink` callback function.
/// MUST be finished with `rewriter_end` which will free the
/// associated memory.
#[no_mangle]
pub extern "C" fn rewriter_new(
    speedreader: *const SpeedReader,
    url: *const c_char,
    url_len: size_t,
    output_sink: unsafe extern "C" fn(*const c_char, size_t, *mut c_void),
    output_sink_user_data: *mut c_void,
) -> *mut CRewriter {
    let url = unwrap_or_ret_null! { to_str!(url, url_len) };
    let speedreader = to_ref!(speedreader);

    let output_sink = ExternOutputSink::new(output_sink, output_sink_user_data);

    let rewriter = unwrap_or_ret_null! { speedreader
        .get_rewriter(
            url,
            output_sink,
        )
    };
    box_to_opaque!(rewriter, CRewriter)
}

/// Set up minimal length of the output content.
#[no_mangle]
pub extern "C" fn rewriter_set_min_out_length(rewriter: *mut CRewriter, min_out_length: i32) {
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);
    rewriter.set_min_out_length(min_out_length);
}

#[no_mangle]
pub extern "C" fn rewriter_set_theme(rewriter: *mut CRewriter, theme: *const c_char) {
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);
    let the_theme = unsafe {
        let c_s = theme;
        str::from_utf8_unchecked(slice::from_raw_parts(c_s as *const u8, strlen(c_s)))
    };
    rewriter.set_theme(the_theme);
}

#[no_mangle]
pub extern "C" fn rewriter_set_font_family(rewriter: *mut CRewriter, font: *const c_char) {
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);
    let the_font = unsafe {
        let c_s = font;
        str::from_utf8_unchecked(slice::from_raw_parts(c_s as *const u8, strlen(c_s)))
    };
    rewriter.set_font_family(the_font);
}

#[no_mangle]
pub extern "C" fn rewriter_set_font_size(rewriter: *mut CRewriter, size: *const c_char) {
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);
    let the_size = unsafe {
        let c_s = size;
        str::from_utf8_unchecked(slice::from_raw_parts(c_s as *const u8, strlen(c_s)))
    };
    rewriter.set_font_size(the_size);
}

#[no_mangle]
pub extern "C" fn rewriter_set_column_width(rewriter: *mut CRewriter, width: *const c_char) {
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);
    let the_width = unsafe {
        let c_s = width;
        str::from_utf8_unchecked(slice::from_raw_parts(c_s as *const u8, strlen(c_s)))
    };
    rewriter.set_column_width(the_width);
}

/// Write a new chunk of data (byte array) to the rewriter instance.
#[no_mangle]
pub extern "C" fn rewriter_write(
    rewriter: *mut CRewriter,
    chunk: *const c_char,
    chunk_len: size_t,
) -> c_int {
    let chunk = to_bytes!(chunk, chunk_len);
    let rewriter: &mut Box<dyn SpeedReaderProcessor> = leak_void_to_box!(rewriter);

    let mut rewriter = AssertUnwindSafe(rewriter);

    let res = panic::catch_unwind(move || rewriter.write(chunk));

    let res = match res {
        Ok(v) => v,
        Err(_err) => {
            // crate::errors::LAST_ERROR.with(|cell| *cell.borrow_mut() = Some(err.into()));
            return -1;
        }
    };
    unwrap_or_ret_err_code! { res };
    0
}

/// Complete rewriting for this instance.
/// Will free memory used by the rewriter.
/// Calling twice will cause panic.
#[no_mangle]
pub extern "C" fn rewriter_end(rewriter: *mut CRewriter) -> c_int {
    // Clean up the memory by converting the pointer back into a Box and letting
    // the Box be dropped at the end of the function
    let mut rewriter: Box<Box<dyn SpeedReaderProcessor>> = void_to_box!(rewriter);
    unwrap_or_ret_err_code! { rewriter.end() };
    0
}

#[no_mangle]
pub extern "C" fn rewriter_free(rewriter: *mut CRewriter) {
    // Clean up the memory by converting the pointer back
    // into a Box and letting the Box be dropped.
    drop(void_to_box!(rewriter));
}
