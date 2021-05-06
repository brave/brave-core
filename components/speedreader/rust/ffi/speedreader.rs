use super::*;
use libc::c_void;
use std::panic::{self, AssertUnwindSafe};
use std::any::Any;

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

/// Indicate type of rewriter that would be used based on existing
/// configuration. `RewrtierUnknown` indicates that no configuration was found
/// for the provided parameters.
/// Also used to ask for a specific type of rewriter if desired; passing
/// `RewriterUnknown` tells SpeedReader to look the type up by configuration
/// and use heuristics-based one if not found otherwise.
#[repr(C)]
pub enum CRewriterType {
    RewriterStreaming,
    RewriterHeuristics,
    RewriterReadability,
    RewriterUnknown,
}

impl CRewriterType {
    fn to_rewriter_type(&self) -> Option<RewriterType> {
        match &self {
            CRewriterType::RewriterStreaming => Some(RewriterType::Streaming),
            CRewriterType::RewriterHeuristics => Some(RewriterType::Heuristics),
            CRewriterType::RewriterReadability => Some(RewriterType::Readability),
            CRewriterType::RewriterUnknown => None,
        }
    }
}

impl From<RewriterType> for CRewriterType {
    fn from(r_type: RewriterType) -> Self {
        match r_type {
            RewriterType::Streaming => CRewriterType::RewriterStreaming,
            RewriterType::Heuristics => CRewriterType::RewriterHeuristics,
            RewriterType::Readability => CRewriterType::RewriterReadability,
            RewriterType::Unknown => CRewriterType::RewriterUnknown,
        }
    }
}

/// Opaque structure to have the minimum amount of type safety across the FFI.
/// Only replaces c_void
#[repr(C)]
pub struct CRewriter {
    _private: [u8; 0],
}

/// Opaque structure to have the minimum amount of type safety across the FFI.
/// Only replaces c_void
#[repr(C)]
pub struct CRewriterConfig {
    _private: [u8; 0],
}

/// New instance of SpeedReader. Loads the default configuration and rewriting
/// whitelists. Must be freed by calling `speedreader_free`.
#[no_mangle]
pub extern "C" fn speedreader_new() -> *mut SpeedReader {
    to_ptr_mut(SpeedReader::default())
}

/// New instance of SpeedReader using deserialized whitelist
#[no_mangle]
pub extern "C" fn with_whitelist(
    whitelist_data: *const c_char,
    whitelist_data_size: size_t,
) -> *mut SpeedReader {
    let whitelist_data: &[u8] =
        unsafe { std::slice::from_raw_parts(whitelist_data as *const u8, whitelist_data_size) };
    let whitelist = unwrap_or_ret_null! { whitelist::Whitelist::deserialize(whitelist_data) };
    to_ptr_mut(SpeedReader::with_whitelist(whitelist))
}

/// Checks if the provided URL matches whitelisted readable URLs.
#[no_mangle]
pub extern "C" fn url_readable(
    speedreader: *const SpeedReader,
    url: *const c_char,
    url_len: size_t,
) -> bool {
    let url = unwrap_or_ret! { to_str!(url, url_len), false };
    let speedreader = to_ref!(speedreader);
    speedreader.url_readable(url).unwrap_or(false)
}

/// Returns type of SpeedReader that would be applied by default for the given
/// URL. `RewriterUnknown` if no match in the whitelist.
#[no_mangle]
pub extern "C" fn find_type(
    speedreader: *const SpeedReader,
    url: *const c_char,
    url_len: size_t,
) -> CRewriterType {
    let url = unwrap_or_ret! { to_str!(url, url_len), CRewriterType::RewriterUnknown };
    let speedreader = to_ref!(speedreader);
    let rewriter_type = speedreader.get_rewriter_type_from_list(url);
    CRewriterType::from(rewriter_type)
}

#[no_mangle]
pub extern "C" fn speedreader_free(speedreader: *mut SpeedReader) {
    assert_not_null!(speedreader);
    drop(to_box!(speedreader));
}

#[no_mangle]
pub extern "C" fn get_rewriter_opaque_config(
    speedreader: *const SpeedReader,
    url: *const c_char,
    url_len: size_t
) -> *mut CRewriterConfig {
    let url = unwrap_or_ret_null! { to_str!(url, url_len) };
    let speedreader = to_ref!(speedreader);

    let opaque_config = speedreader.get_opaque_config(url);
    box_to_opaque!(opaque_config, CRewriterConfig)
}

/// Returns SpeedReader rewriter instance for the given URL. If provided
/// `rewriter_type` is `RewriterUnknown`, will look it up in the whitelist
/// and default to heuristics-based rewriter if none found in the whitelist.
/// Returns NULL if no URL provided or initialization fails.
/// Results of rewriting sent to `output_sink` callback function.
/// MUST be finished with `rewriter_end` which will free
/// associated memory.
#[no_mangle]
pub extern "C" fn rewriter_new(
    speedreader: *const SpeedReader,
    url: *const c_char,
    url_len: size_t,
    output_sink: unsafe extern "C" fn(*const c_char, size_t, *mut c_void),
    output_sink_user_data: *mut c_void,
    rewriter_opaque_config: *mut CRewriterConfig,
    rewriter_type: CRewriterType,
) -> *mut CRewriter {
    let url = unwrap_or_ret_null! { to_str!(url, url_len) };
    let speedreader = to_ref!(speedreader);

    let opaque_config: &Box<dyn Any> = leak_void_to_box!(rewriter_opaque_config);

    let output_sink = ExternOutputSink::new(output_sink, output_sink_user_data);

    let rewriter = unwrap_or_ret_null! { speedreader
        .get_rewriter(
            url,
            opaque_config,
            output_sink,
            rewriter_type.to_rewriter_type(),
        )
    };
    box_to_opaque!(rewriter, CRewriter)
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

    let res = panic::catch_unwind(move || {
      rewriter.write(chunk)
    });

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
    void_to_box!(rewriter);
}

#[no_mangle]
pub extern "C" fn free_rewriter_opaque_config(config: *mut CRewriterConfig) {
    // Clean up the memory by converting the pointer back
    // into a Box and letting the Box be dropped.
    void_to_box!(config);
}
