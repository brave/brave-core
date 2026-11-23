#![no_std]
//! Minimal safe wrapper around zstd-sys.
//!
//! This crates provides a minimal translation of the [zstd-sys] methods.
//! For a more comfortable high-level library, see the [zstd] crate.
//!
//! [zstd-sys]: https://crates.io/crates/zstd-sys
//! [zstd]: https://crates.io/crates/zstd
//!
//! Most of the functions here map 1-for-1 to a function from
//! [the C zstd library][zstd-c] mentioned in their descriptions.
//! Check the [source documentation][doc] for more information on their
//! behaviour.
//!
//! [doc]: https://facebook.github.io/zstd/zstd_manual.html
//! [zstd-c]: https://facebook.github.io/zstd/
//!
//! Features denoted as experimental in the C library are hidden behind an
//! `experimental` feature.
#![cfg_attr(feature = "doc-cfg", feature(doc_cfg))]

// TODO: Use alloc feature instead to implement stuff for Vec
// TODO: What about Cursor?
#[cfg(feature = "std")]
extern crate std;

#[cfg(test)]
mod tests;

#[cfg(feature = "seekable")]
pub mod seekable;

// Re-export zstd-sys
pub use zstd_sys;

/// How to compress data.
pub use zstd_sys::ZSTD_strategy as Strategy;

/// Frame progression state.
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub use zstd_sys::ZSTD_frameProgression as FrameProgression;

/// Reset directive.
// pub use zstd_sys::ZSTD_ResetDirective as ResetDirective;
use core::ffi::{c_char, c_int, c_ulonglong, c_void};

use core::marker::PhantomData;
use core::num::{NonZeroU32, NonZeroU64};
use core::ops::{Deref, DerefMut};
use core::ptr::NonNull;
use core::str;

include!("constants.rs");

#[cfg(feature = "experimental")]
include!("constants_experimental.rs");

#[cfg(feature = "seekable")]
include!("constants_seekable.rs");

/// Represents the compression level used by zstd.
pub type CompressionLevel = i32;

/// Represents a possible error from the zstd library.
pub type ErrorCode = usize;

/// Wrapper result around most zstd functions.
///
/// Either a success code (usually number of bytes written), or an error code.
pub type SafeResult = Result<usize, ErrorCode>;

/// The error code zstd itself returns when an allocation fails
/// (`-ZSTD_error_memory_allocation` as a `size_t`), for the places where a
/// context is allocated on the Rust side of a call.
#[cfg(feature = "experimental")]
const MEMORY_ALLOCATION_ERROR: ErrorCode =
    (zstd_sys::ZSTD_ErrorCode::ZSTD_error_memory_allocation as usize)
        .wrapping_neg();

/// The error code zstd itself returns for a parameter it does not support
/// (`-ZSTD_error_parameter_unsupported` as a `size_t`), for the parameters
/// that are refused on the Rust side of a call.
#[cfg(feature = "experimental")]
const PARAMETER_UNSUPPORTED_ERROR: ErrorCode =
    (zstd_sys::ZSTD_ErrorCode::ZSTD_error_parameter_unsupported as usize)
        .wrapping_neg();

/// The error code zstd itself returns for an operation the context is in no
/// state to perform (`-ZSTD_error_stage_wrong` as a `size_t`), for the places
/// where that has to be decided on the Rust side of a call.
const STAGE_WRONG_ERROR: ErrorCode =
    (zstd_sys::ZSTD_ErrorCode::ZSTD_error_stage_wrong as usize).wrapping_neg();

/// The error code `ZSTD_compress2()` returns when the destination is too
/// small, leaving its frame in progress (`-ZSTD_error_dstSize_tooSmall`).
const DST_SIZE_TOO_SMALL_ERROR: ErrorCode =
    (zstd_sys::ZSTD_ErrorCode::ZSTD_error_dstSize_tooSmall as usize)
        .wrapping_neg();

/// The largest frame zstd compresses on the calling thread when its size is
/// known (`ZSTDMT_JOBSIZE_MIN` in zstd 1.5; older versions used larger
/// values, which only makes this cautious).
const ZSTDMT_JOBSIZE_MIN: usize = 512 << 10;

/// Indicates an error happened when parsing the frame content size.
///
/// The stream may be corrupted, or the given frame prefix was too small.
#[derive(Debug)]
pub struct ContentSizeError;

impl core::fmt::Display for ContentSizeError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.write_str("Could not get content size")
    }
}

/// Returns true if code represents error.
fn is_error(code: usize) -> bool {
    // Safety: Just FFI
    unsafe { zstd_sys::ZSTD_isError(code) != 0 }
}

/// Parse the result code
///
/// Returns the number of bytes written if the code represents success,
/// or the error message code otherwise.
fn parse_code(code: usize) -> SafeResult {
    if !is_error(code) {
        Ok(code)
    } else {
        Err(code)
    }
}

/// Parse a content size value.
///
/// zstd uses 2 special content size values to indicate either unknown size or parsing error.
fn parse_content_size(
    content_size: u64,
) -> Result<Option<u64>, ContentSizeError> {
    match content_size {
        CONTENTSIZE_ERROR => Err(ContentSizeError),
        CONTENTSIZE_UNKNOWN => Ok(None),
        other => Ok(Some(other)),
    }
}

fn ptr_void(src: &[u8]) -> *const c_void {
    src.as_ptr() as *const c_void
}

fn ptr_mut_void(dst: &mut (impl WriteBuf + ?Sized)) -> *mut c_void {
    dst.as_mut_ptr() as *mut c_void
}

/// Returns the ZSTD version.
///
/// Returns `major * 10_000 + minor * 100 + patch`.
/// So 1.5.3 would be returned as `10_503`.
pub fn version_number() -> u32 {
    // Safety: Just FFI
    unsafe { zstd_sys::ZSTD_versionNumber() as u32 }
}

/// Returns a string representation of the ZSTD version.
///
/// For example "1.5.3".
pub fn version_string() -> &'static str {
    // Safety: Assumes `ZSTD_versionString` returns a valid utf8 string.
    unsafe { c_char_to_str(zstd_sys::ZSTD_versionString()) }
}

/// Returns the minimum (fastest) compression level supported.
///
/// This is likely going to be a _very_ large negative number.
pub fn min_c_level() -> CompressionLevel {
    // Safety: Just FFI
    unsafe { zstd_sys::ZSTD_minCLevel() as CompressionLevel }
}

/// Returns the maximum (slowest) compression level supported.
pub fn max_c_level() -> CompressionLevel {
    // Safety: Just FFI
    unsafe { zstd_sys::ZSTD_maxCLevel() as CompressionLevel }
}

/// Wraps the `ZSTD_compress` function.
///
/// This will try to compress `src` entirely and write the result to `dst`, returning the number of
/// bytes written. If `dst` is too small to hold the compressed content, an error will be returned.
///
/// For streaming operations that don't require to store the entire input/output in memory, see
/// `compress_stream`.
pub fn compress<C: WriteBuf + ?Sized>(
    dst: &mut C,
    src: &[u8],
    compression_level: CompressionLevel,
) -> SafeResult {
    // Safety: ZSTD_compress indeed returns how many bytes have been written.
    unsafe {
        dst.write_from(|buffer, capacity| {
            parse_code(zstd_sys::ZSTD_compress(
                buffer,
                capacity,
                ptr_void(src),
                src.len(),
                compression_level,
            ))
        })
    }
}

/// Wraps the `ZSTD_decompress` function.
///
/// This is a one-step decompression (not streaming).
///
/// You will need to make sure `dst` is large enough to store all the decompressed content, or an
/// error will be returned.
///
/// If decompression was a success, the number of bytes written will be returned.
pub fn decompress<C: WriteBuf + ?Sized>(
    dst: &mut C,
    src: &[u8],
) -> SafeResult {
    // Safety: ZSTD_decompress indeed returns how many bytes have been written.
    unsafe {
        dst.write_from(|buffer, capacity| {
            parse_code(zstd_sys::ZSTD_decompress(
                buffer,
                capacity,
                ptr_void(src),
                src.len(),
            ))
        })
    }
}

/// Wraps the `ZSTD_getDecompressedSize` function.
///
/// Returns `None` if the size could not be found, or if the content is actually empty.
#[deprecated(note = "Use ZSTD_getFrameContentSize instead")]
pub fn get_decompressed_size(src: &[u8]) -> Option<NonZeroU64> {
    // Safety: Just FFI
    NonZeroU64::new(unsafe {
        zstd_sys::ZSTD_getDecompressedSize(ptr_void(src), src.len()) as u64
    })
}

/// Maximum compressed size in worst case single-pass scenario
pub fn compress_bound(src_size: usize) -> usize {
    // Safety: Just FFI
    unsafe { zstd_sys::ZSTD_compressBound(src_size) }
}

/// Remembers an error that may have left a context in a state zstd considers
/// undefined.
///
/// zstd.h says of both `ZSTD_compressStream2()` and `ZSTD_decompressStream()`
/// that "if an operation ends with an error, it may leave [the context] in an
/// undefined state", and that calling them again on such a state is undefined
/// behaviour - the context has to be reset first. So once a streaming
/// operation fails, refuse to run another one until something resets the
/// context, and hand the original error back instead.
#[derive(Clone, Debug, Default)]
struct Poison(Option<ErrorCode>);

impl Poison {
    /// Fails with the error that poisoned the context, if there was one.
    fn guard(&self) -> Result<(), ErrorCode> {
        match self.0 {
            Some(code) => Err(code),
            None => Ok(()),
        }
    }

    /// Remembers `res` if it failed, and passes it through.
    fn record(&mut self, res: SafeResult) -> SafeResult {
        if let Err(code) = res {
            self.0 = Some(code);
        }
        res
    }

    /// The context was reset, so it is usable again.
    fn clear(&mut self) {
        self.0 = None;
    }
}

/// What a `CCtx` knows about its worker threads (`NbWorkers`).
#[derive(Clone, Copy, Debug, Default)]
struct Workers {
    /// `NbWorkers` was ever set to a non-zero value, see `guard_own_tables`.
    ever: bool,
    /// `NbWorkers` is currently set to a non-zero value.
    requested: bool,
    /// A frame was started while `NbWorkers` was set, and is not finished.
    ///
    /// Its jobs may be running on the worker threads, reading the context's
    /// dictionary and job table. zstd up to 1.5.7 does not wait for them when
    /// the frame is abandoned: not on a session reset, not before a short
    /// frame (which it compresses on the calling thread), and not before
    /// freeing the dictionary when the context is freed. So the frame is
    /// drained first, see `CCtx::drain()`.
    pending: bool,
    /// `ZSTD_e_end` was given for the `pending` frame. zstd then answers
    /// `ZSTD_e_continue` by resetting the session without waiting for the
    /// jobs, so that is refused here first.
    ended: bool,
    /// A `ZSTD_e_end` call took all of its input: zstd made the frame's last
    /// job. It would hand any new input to a worker as a job after the last
    /// one, which waits forever, so that is refused here first.
    last: bool,
    /// The size pledged for the next frame is short (see `short`).
    pledged_short: bool,
    /// The `pending` frame is known to be compressed on the calling thread:
    /// its size is known (pledged, or given whole with `ZSTD_e_end`), and is
    /// short (zstd hands out no jobs for a frame of at most
    /// `ZSTDMT_JOBSIZE_MIN`).
    short: bool,
    /// The `pending` frame may have handed jobs to the worker threads: it
    /// took some input, and is not `short`.
    jobs: bool,
    /// A job failed. zstd up to 1.5.7 can then keep a job it had prepared but
    /// not handed to a worker yet (`jobReady`), after clearing it: the next
    /// multithreaded frame hands that empty job to a worker, which crashes.
    /// That stays in the context's multithreading state, which only freeing
    /// the context frees, so no other multithreaded frame is started on it.
    broken: bool,
    /// `StableOutBuffer` is enabled. Draining needs an output buffer of its
    /// own, which zstd then refuses, so it is not allowed with workers.
    stable_out: bool,
}

/// Compression context
///
/// It is recommended to allocate a single context per thread and re-use it
/// for many compression operations.
///
/// # Worker threads and leaks
///
/// With `CParameter::NbWorkers`, the jobs of a frame run on worker threads,
/// which read the dictionary. Anything that abandons a frame in progress
/// waits for them, dropping the context included (see
/// [`CCtx::wait_for_workers()`]). A context that is leaked instead
/// (`mem::forget`, `Box::leak`, a reference cycle) never waits: with a
/// dictionary or prefix it only *borrows* ([`CCtx::ref_cdict()`],
/// [`CCtx::ref_prefix()`]), the workers may then read it after it is
/// dropped. Do not leak a context that is compressing with worker threads
/// from borrowed data. A dictionary loaded with
/// [`CCtx::load_dictionary()`] is copied, and leaks with the context.
pub struct CCtx<'a>(
    NonNull<zstd_sys::ZSTD_CCtx>,
    PhantomData<&'a ()>,
    Poison,
    Workers,
);

impl Default for CCtx<'_> {
    fn default() -> Self {
        CCtx::create()
    }
}

impl<'a> CCtx<'a> {
    /// Tries to create a new context.
    ///
    /// Returns `None` if zstd returns a NULL pointer - may happen if allocation fails.
    pub fn try_create() -> Option<Self> {
        // Safety: Just FFI
        Some(CCtx(
            NonNull::new(unsafe { zstd_sys::ZSTD_createCCtx() })?,
            PhantomData,
            Poison::default(),
            Workers::default(),
        ))
    }

    /// Wrap `ZSTD_createCCtx`
    ///
    /// # Panics
    ///
    /// If zstd returns a NULL pointer.
    pub fn create() -> Self {
        Self::try_create()
            .expect("zstd returned null pointer when creating new context")
    }

    /// Ends the session of a one-shot compression, and passes `res` through.
    ///
    /// zstd.h has these calls imply a reset, but most of them leave the stream
    /// stage alone (and their source size behind as the pledged one), while
    /// `ZSTD_compress2()` stays mid-frame when it fails. A later stream call
    /// would then carry on from a state that still points at buffers and
    /// dictionaries borrowed for that one call. So reset explicitly: the next
    /// stream starts from scratch.
    fn finish_one_shot(&mut self, res: SafeResult) -> SafeResult {
        self.reset(ResetDirective::SessionOnly).and(res)
    }

    /// Wraps the `ZSTD_compressCCtx()` function
    pub fn compress<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
        compression_level: CompressionLevel,
    ) -> SafeResult {
        // The one-shot calls run on the calling thread, but end by resetting
        // the session (see `finish_one_shot`), and overwrite the parameters a
        // multithreaded frame left open was started with.
        self.drain();
        // Safety: ZSTD_compressCCtx returns how many bytes were written.
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_compressCCtx(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                    compression_level,
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Wraps the `ZSTD_compress2()` function.
    pub fn compress2<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
    ) -> SafeResult {
        if !self.3.requested {
            // Safety: ZSTD_compress2 returns how many bytes were written.
            let res = unsafe {
                dst.write_from(|buffer, capacity| {
                    parse_code(zstd_sys::ZSTD_compress2(
                        self.0.as_ptr(),
                        buffer,
                        capacity,
                        ptr_void(src),
                        src.len(),
                    ))
                })
            };
            // Unlike the others, this starts by resetting the session, and
            // ends it when it succeeds. When it fails, it stays mid-frame:
            // make streaming calls wait for an explicit reset.
            self.2.clear();
            return self.2.record(res);
        }

        // With worker threads, a frame this leaves in progress must be one
        // that can be waited for later, see `drain()`. `ZSTD_compress2()`
        // runs its frame with stable buffers, and zstd would then refuse any
        // other buffer: do what it does, but with ordinary ones. That costs
        // nothing to a multithreaded frame, which copies its input and output
        // anyway. A short frame, compressed on the calling thread, goes
        // straight to the destination when it can hold the whole frame, and
        // through a copy otherwise.
        // Waiting for the frame in progress may find that one of its jobs
        // failed: check `broken` after.
        self.reset(ResetDirective::SessionOnly)?;
        self.guard_broken()?;
        let mut raw = Ok(0);
        let mut taken = 0;
        // Safety: ZSTD_compressStream2 reports how many bytes it wrote in
        // `output.pos`.
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                let mut output = zstd_sys::ZSTD_outBuffer {
                    dst: buffer,
                    size: capacity,
                    pos: 0,
                };
                let mut input = zstd_sys::ZSTD_inBuffer {
                    src: ptr_void(src),
                    size: src.len(),
                    pos: 0,
                };
                raw = parse_code(zstd_sys::ZSTD_compressStream2(
                    self.0.as_ptr(),
                    &mut output,
                    &mut input,
                    zstd_sys::ZSTD_EndDirective::ZSTD_e_end,
                ));
                taken = input.pos;
                match raw {
                    Ok(0) => Ok(output.pos),
                    // As `ZSTD_compress2()`: the frame stays in progress.
                    Ok(_) => Err(DST_SIZE_TOO_SMALL_ERROR),
                    Err(code) => Err(code),
                }
            })
        };
        self.track_frame(
            raw,
            zstd_sys::ZSTD_EndDirective::ZSTD_e_end,
            src.len(),
            taken,
        );
        self.2.record(res)
    }

    /// Wraps the `ZSTD_compress_usingDict()` function.
    pub fn compress_using_dict<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
        dict: &[u8],
        compression_level: CompressionLevel,
    ) -> SafeResult {
        // See `compress()`.
        self.drain();
        // Safety: ZSTD_compress_usingDict returns how many bytes were written.
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_compress_usingDict(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                    ptr_void(dict),
                    dict.len(),
                    compression_level,
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Wraps the `ZSTD_compress_usingCDict()` function.
    pub fn compress_using_cdict<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
        cdict: &CDict<'_>,
    ) -> SafeResult {
        // See `compress()`.
        self.drain();
        // Safety: ZSTD_compress_usingCDict returns how many bytes were written.
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_compress_usingCDict(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                    cdict.0.as_ptr(),
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Initializes the context with the given compression level.
    ///
    /// This is equivalent to running:
    /// * `reset()`
    /// * `set_parameter(CompressionLevel, compression_level)`
    ///
    /// Abandons the current frame, if any. With worker threads, this first
    /// waits for its jobs, see [`CCtx::wait_for_workers()`].
    pub fn init(&mut self, compression_level: CompressionLevel) -> SafeResult {
        // This also resets the session and unloads the dictionary.
        self.drain();
        self.2.clear();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_initCStream(self.0.as_ptr(), compression_level)
        };
        self.2.record(parse_code(code))
    }

    /// Wraps the `ZSTD_initCStream_srcSize()` function.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    #[deprecated]
    pub fn init_src_size(
        &mut self,
        compression_level: CompressionLevel,
        pledged_src_size: u64,
    ) -> SafeResult {
        self.drain();
        self.2.clear();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_initCStream_srcSize(
                self.0.as_ptr(),
                compression_level as c_int,
                pledged_src_size as c_ulonglong,
            )
        };
        parse_code(code)
    }

    /// Wraps the `ZSTD_initCStream_usingDict()` function.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    #[deprecated]
    pub fn init_using_dict(
        &mut self,
        dict: &[u8],
        compression_level: CompressionLevel,
    ) -> SafeResult {
        self.drain();
        self.2.clear();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_initCStream_usingDict(
                self.0.as_ptr(),
                ptr_void(dict),
                dict.len(),
                compression_level,
            )
        };
        parse_code(code)
    }

    /// Wraps the `ZSTD_initCStream_usingCDict()` function.
    ///
    /// Dictionary must outlive the context.
    ///
    /// ```compile_fail,E0515
    /// # #![allow(deprecated)]
    /// fn compressor() -> zstd_safe::CCtx<'static> {
    ///     let cdict = zstd_safe::CDict::create(b"dictionary", 3);
    ///     let mut cctx = zstd_safe::CCtx::create();
    ///     cctx.init_using_cdict(&cdict).unwrap();
    ///     cctx // `cdict` would be dropped while `cctx` still points at it.
    /// }
    /// ```
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    #[deprecated]
    pub fn init_using_cdict<'b>(&mut self, cdict: &'a CDict<'b>) -> SafeResult
    where
        'b: 'a, // Dictionary outlives the stream.
    {
        self.drain();
        self.2.clear();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_initCStream_usingCDict(
                self.0.as_ptr(),
                cdict.0.as_ptr(),
            )
        };
        self.2.record(parse_code(code))
    }

    /// Tries to load a dictionary.
    ///
    /// The dictionary content will be copied internally and does not need to be kept alive after
    /// calling this function.
    ///
    /// If you need to use the same dictionary for multiple contexts, it may be more efficient to
    /// create a `CDict` first, then loads that.
    ///
    /// The dictionary will apply to all compressed frames, until a new dictionary is set.
    ///
    /// Like zstd, this fails with a `stage_wrong` error while a frame is in
    /// progress: reset the session first (see [`CCtx::wait_for_workers()`]).
    pub fn load_dictionary(&mut self, dict: &[u8]) -> SafeResult {
        self.guard_workers()?;
        // Safety: Just FFI
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_loadDictionary(
                self.0.as_ptr(),
                ptr_void(dict),
                dict.len(),
            )
        })
    }

    /// Wraps the `ZSTD_CCtx_refCDict()` function.
    ///
    /// Dictionary must outlive the context. With worker threads, do not leak
    /// the context, see [its documentation](CCtx#worker-threads-and-leaks).
    ///
    /// Like zstd, this fails with a `stage_wrong` error while a frame is in
    /// progress: reset the session first (see [`CCtx::wait_for_workers()`]).
    pub fn ref_cdict<'b>(&mut self, cdict: &CDict<'b>) -> SafeResult
    where
        'b: 'a,
    {
        self.guard_workers()?;
        // Safety: Just FFI
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_refCDict(self.0.as_ptr(), cdict.0.as_ptr())
        })
    }

    /// Return to "no-dictionary" mode.
    ///
    /// This will disable any dictionary/prefix previously registered for future frames.
    ///
    /// Like zstd, this fails with a `stage_wrong` error while a frame is in
    /// progress: reset the session first (see [`CCtx::wait_for_workers()`]).
    pub fn disable_dictionary(&mut self) -> SafeResult {
        self.guard_workers()?;
        // Safety: Just FFI
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_loadDictionary(
                self.0.as_ptr(),
                core::ptr::null(),
                0,
            )
        })
    }

    /// Use some prefix as single-use dictionary for the next compressed frame.
    ///
    /// Just like a dictionary, decompression will need to be given the same prefix.
    ///
    /// This is best used if the "prefix" looks like the data to be compressed.
    /// With worker threads, do not leak the context, see
    /// [its documentation](CCtx#worker-threads-and-leaks).
    ///
    /// Like zstd, this fails with a `stage_wrong` error while a frame is in
    /// progress: reset the session first (see [`CCtx::wait_for_workers()`]).
    pub fn ref_prefix<'b>(&mut self, prefix: &'b [u8]) -> SafeResult
    where
        'b: 'a,
    {
        self.guard_workers()?;
        // Safety: Just FFI
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_refPrefix(
                self.0.as_ptr(),
                ptr_void(prefix),
                prefix.len(),
            )
        })
    }

    /// Performs a step of a streaming compression operation.
    ///
    /// This will read some data from `input` and/or write some data to `output`.
    ///
    /// # Returns
    ///
    /// A hint for the "ideal" amount of input data to provide in the next call.
    ///
    /// This hint is only for performance purposes.
    ///
    /// Wraps the `ZSTD_compressStream()` function.
    pub fn compress_stream<C: WriteBuf + ?Sized>(
        &mut self,
        output: &mut OutBuffer<'_, C>,
        input: &mut InBuffer<'_>,
    ) -> SafeResult {
        use zstd_sys::ZSTD_EndDirective::ZSTD_e_continue;
        self.2.guard()?;
        if let Some(res) =
            self.before_stream(input.pos == input.src.len(), ZSTD_e_continue)
        {
            // Any input is welcome.
            return res.map(|_| Self::in_size());
        }
        let remaining = input.src.len().saturating_sub(input.pos);
        let mut output = output.wrap();
        let mut input = input.wrap();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_compressStream(
                self.0.as_ptr(),
                ptr_mut(&mut output),
                ptr_mut(&mut input),
            )
        };
        let taken = input.buf.pos.saturating_sub(input.parent.pos);
        let res = output.check(parse_code(code));
        let res = self.2.record(input.check(res));
        self.track_frame(res, ZSTD_e_continue, remaining, taken);
        res
    }

    /// Performs a step of a streaming compression operation.
    ///
    /// This will read some data from `input` and/or write some data to `output`.
    ///
    /// The `end_op` directive can be used to specify what to do after: nothing special, flush
    /// internal buffers, or end the frame.
    ///
    /// # Returns
    ///
    /// An lower bound for the amount of data that still needs to be flushed out.
    ///
    /// This is useful when flushing or ending the frame: you need to keep calling this function
    /// until it returns 0.
    ///
    /// Wraps the `ZSTD_compressStream2()` function.
    pub fn compress_stream2<C: WriteBuf + ?Sized>(
        &mut self,
        output: &mut OutBuffer<'_, C>,
        input: &mut InBuffer<'_>,
        end_op: zstd_sys::ZSTD_EndDirective,
    ) -> SafeResult {
        self.2.guard()?;
        if let Some(res) =
            self.before_stream(input.pos == input.src.len(), end_op)
        {
            return res;
        }
        let remaining = input.src.len().saturating_sub(input.pos);
        let mut output = output.wrap();
        let mut input = input.wrap();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_compressStream2(
                self.0.as_ptr(),
                ptr_mut(&mut output),
                ptr_mut(&mut input),
                end_op,
            )
        };
        let taken = input.buf.pos.saturating_sub(input.parent.pos);
        let res = output.check(parse_code(code));
        let res = self.2.record(input.check(res));
        self.track_frame(res, end_op, remaining, taken);
        res
    }

    /// Flush any intermediate buffer.
    ///
    /// To fully flush, you should keep calling this function until it returns `Ok(0)`.
    ///
    /// Wraps the `ZSTD_flushStream()` function.
    pub fn flush_stream<C: WriteBuf + ?Sized>(
        &mut self,
        output: &mut OutBuffer<'_, C>,
    ) -> SafeResult {
        use zstd_sys::ZSTD_EndDirective::ZSTD_e_flush;
        self.2.guard()?;
        if let Some(res) = self.before_stream(true, ZSTD_e_flush) {
            return res;
        }
        let mut output = output.wrap();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_flushStream(self.0.as_ptr(), ptr_mut(&mut output))
        };
        let res = self.2.record(output.check(parse_code(code)));
        self.track_frame(res, ZSTD_e_flush, 0, 0);
        res
    }

    /// Ends the stream.
    ///
    /// You should keep calling this function until it returns `Ok(0)`.
    ///
    /// Wraps the `ZSTD_endStream()` function.
    pub fn end_stream<C: WriteBuf + ?Sized>(
        &mut self,
        output: &mut OutBuffer<'_, C>,
    ) -> SafeResult {
        use zstd_sys::ZSTD_EndDirective::ZSTD_e_end;
        self.2.guard()?;
        if let Some(res) = self.before_stream(true, ZSTD_e_end) {
            return res;
        }
        let mut output = output.wrap();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_endStream(self.0.as_ptr(), ptr_mut(&mut output))
        };
        let res = self.2.record(output.check(parse_code(code)));
        self.track_frame(res, ZSTD_e_end, 0, 0);
        res
    }

    /// Decides whether a streaming call may reach zstd, see `Workers`.
    ///
    /// Returns what the call should return instead when it may not.
    fn before_stream(
        &self,
        no_input: bool,
        end_op: zstd_sys::ZSTD_EndDirective,
    ) -> Option<SafeResult> {
        use zstd_sys::ZSTD_EndDirective::{ZSTD_e_continue, ZSTD_e_end};
        if !self.3.requested {
            return None;
        }
        if self.3.broken {
            return Some(Err(STAGE_WRONG_ERROR));
        }
        if self.3.pending {
            if (self.3.ended && end_op == ZSTD_e_continue)
                || (self.3.last && !no_input)
            {
                return Some(Err(STAGE_WRONG_ERROR));
            }
        } else if no_input && end_op != ZSTD_e_end {
            // Starting a frame with nothing in it. If it was then ended,
            // zstd up to 1.5.7 would hand an empty job to a worker, which its
            // own wait on errors (`ZSTDMT_waitForAllJobsCompleted`) takes for
            // done before the worker even starts. `drain()` does wait for it,
            // so this is only a safeguard: wait for some input, nothing would
            // have been written yet anyway. (A frame that starts with
            // `ZSTD_e_end` takes the size of its input, so an empty one is
            // compressed on the calling thread.)
            return Some(Ok(0));
        }
        None
    }

    /// Records what a streaming call left open, see `Workers`.
    ///
    /// `given` is how much input the call was given, and `taken` how much of
    /// it zstd took.
    fn track_frame(
        &mut self,
        res: SafeResult,
        end_op: zstd_sys::ZSTD_EndDirective,
        given: usize,
        taken: usize,
    ) {
        use zstd_sys::ZSTD_EndDirective::ZSTD_e_end;
        // A pledged size is for the frame this call started or continued.
        let pledged_short = core::mem::take(&mut self.3.pledged_short);
        if !self.3.requested {
            return;
        }
        if !self.3.pending {
            // This call started the frame. Given whole, its size is the
            // input's, whatever was pledged.
            self.3.short = if end_op == ZSTD_e_end {
                given <= ZSTDMT_JOBSIZE_MIN
            } else {
                pledged_short
            };
        }
        self.3.jobs |= taken > 0 && !self.3.short;
        match res {
            Ok(0) if end_op == ZSTD_e_end => self.end_frame(),
            Ok(_) => {
                self.3.pending = true;
                if end_op == ZSTD_e_end {
                    self.3.ended = true;
                    self.3.last |= taken == given;
                }
            }
            // When a job failed, zstd waited for the others and reset the
            // session (refused directives and buffers never get that far).
            // Otherwise, there are no jobs. Either way, nothing is left to
            // wait for.
            Err(_) => {
                self.3.broken |= self.3.jobs;
                self.end_frame();
            }
        }
    }

    /// Records that no frame is in progress anymore.
    fn end_frame(&mut self) {
        self.3.pending = false;
        self.3.ended = false;
        self.3.last = false;
        self.3.short = false;
        self.3.jobs = false;
    }

    /// Waits for the jobs of the frame in progress, if any, so that it can
    /// be abandoned.
    ///
    /// Asking zstd to flush everything waits for every job to be done and
    /// its output flushed (here to a scratch buffer, and thrown away). This
    /// also hands a worker the job zstd prepared but could not hand out yet.
    /// That compresses what was left in zstd's input buffer as one more
    /// job: wasted work, but bounded by one job. Starting a new frame would
    /// avoid it, but zstd up to 1.5.7 can then hand that prepared job out
    /// after clearing it, and a worker crashes.
    fn drain(&mut self) {
        if !self.3.pending {
            return;
        }
        let mut scratch = [0u8; 4096];
        loop {
            let mut output = zstd_sys::ZSTD_outBuffer {
                dst: ptr_mut_void(&mut scratch[..]),
                size: scratch.len(),
                pos: 0,
            };
            let mut input = zstd_sys::ZSTD_inBuffer {
                src: ptr_void(&[]),
                size: 0,
                pos: 0,
            };
            // Safety: Just FFI, with buffers that live through the call.
            let code = unsafe {
                zstd_sys::ZSTD_compressStream2(
                    self.0.as_ptr(),
                    &mut output,
                    &mut input,
                    zstd_sys::ZSTD_EndDirective::ZSTD_e_flush,
                )
            };
            match parse_code(code) {
                Ok(0) => break,
                Ok(_) => continue,
                // A job failed: see `track_frame`.
                Err(_) => {
                    self.3.broken |= self.3.jobs;
                    break;
                }
            }
        }
        self.end_frame();
    }

    /// Fails while a frame is in progress (zstd refuses these calls then).
    ///
    /// zstd up to 1.5.7 would also take them after the session was reset
    /// while worker threads were still running jobs, freeing what they read:
    /// every reset here waits for them first, and this keeps the error the
    /// same whatever the library does.
    fn guard_workers(&self) -> Result<(), ErrorCode> {
        if self.3.pending {
            return Err(STAGE_WRONG_ERROR);
        }
        Ok(())
    }

    /// Fails if a multithreaded call failed on this context, see
    /// `Workers::broken`.
    fn guard_broken(&self) -> Result<(), ErrorCode> {
        if self.3.requested && self.3.broken {
            return Err(STAGE_WRONG_ERROR);
        }
        Ok(())
    }

    /// Abandons the current frame, if any, and waits for the worker threads
    /// to be done with it.
    ///
    /// With `CParameter::NbWorkers`, the jobs of a frame run on worker
    /// threads, reading the dictionary. Anything that abandons a frame in
    /// progress waits for them first: a session reset (and `init()`, which
    /// implies one), a one-shot compression, and dropping the context. This
    /// is the same as `reset(SessionOnly)`. Waiting first compresses what was
    /// left of the frame's input as one more job, and throws the result away.
    ///
    /// After a job failed on a worker thread (for instance because the frame
    /// was given more than its pledged size), the context refuses any
    /// streaming call or `compress2()` while `NbWorkers` is set, and setting
    /// it again (with a `stage_wrong` error): zstd up to 1.5.7, the version
    /// bundled here, can leave a cleared job behind that the next
    /// multithreaded frame would hand to a worker, which crashes. Create a new
    /// context then, or set `NbWorkers` to 0.
    ///
    /// Once a `ZSTD_e_end` call took all of its input, a frame takes no more:
    /// that is refused with a `stage_wrong` error too, instead of the worker
    /// threads waiting forever for a job after the last one.
    pub fn wait_for_workers(&mut self) -> SafeResult {
        self.reset(ResetDirective::SessionOnly)
    }

    /// Returns the size currently used by this context.
    ///
    /// This may change over time.
    pub fn sizeof(&self) -> usize {
        // Safety: Just FFI
        unsafe { zstd_sys::ZSTD_sizeof_CCtx(self.0.as_ptr()) }
    }

    /// Resets the state of the context.
    ///
    /// Depending on the reset mode, it can reset the session, the parameters, or both.
    ///
    /// Resetting the session abandons the frame in progress, if any: with
    /// worker threads, this first waits for its jobs, see
    /// [`CCtx::wait_for_workers()`]. Like zstd, resetting only the parameters
    /// fails with a `stage_wrong` error while a frame is in progress.
    ///
    /// Wraps the `ZSTD_CCtx_reset()` function.
    pub fn reset(&mut self, reset: ResetDirective) -> SafeResult {
        let resets_parameters = !matches!(reset, ResetDirective::SessionOnly);
        if reset.resets_session() {
            self.drain();
        } else {
            self.guard_workers()?;
        }
        let resets_session = reset.resets_session();
        // Safety: Just FFI
        let res = parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_reset(self.0.as_ptr(), reset.as_sys())
        });
        if res.is_ok() && resets_session {
            self.2.clear();
            self.3.pledged_short = false;
        }
        if res.is_ok() && resets_parameters {
            self.3.requested = false;
            self.3.stable_out = false;
        }
        res
    }

    /// Sets a compression parameter.
    ///
    /// Some of these parameters need to be set during de-compression as well.
    ///
    /// Like zstd, changing `NbWorkers` fails with a `stage_wrong` error while
    /// a frame is in progress, and so does setting it to a non-zero value
    /// after a multithreaded call failed (see [`CCtx::wait_for_workers()`]).
    /// `NbWorkers` and `StableOutBuffer` cannot be used together.
    pub fn set_parameter(&mut self, param: CParameter) -> SafeResult {
        match param {
            CParameter::NbWorkers(workers) => {
                // zstd would resize the job table the old jobs still use.
                self.guard_workers()?;
                if workers != 0 {
                    if self.3.broken {
                        return Err(STAGE_WRONG_ERROR);
                    }
                    #[cfg(feature = "experimental")]
                    if self.3.stable_out {
                        return Err(PARAMETER_UNSUPPORTED_ERROR);
                    }
                }
            }
            // Waiting for the workers needs an output buffer of its own.
            #[cfg(feature = "experimental")]
            CParameter::StableOutBuffer(true) if self.3.requested => {
                return Err(PARAMETER_UNSUPPORTED_ERROR);
            }
            _ => (),
        }
        // TODO: Until bindgen properly generates a binding for this, we'll need to do it here.

        #[cfg(feature = "experimental")]
        use zstd_sys::ZSTD_cParameter::{
            ZSTD_c_experimentalParam1 as ZSTD_c_rsyncable,
            ZSTD_c_experimentalParam10 as ZSTD_c_stableOutBuffer,
            ZSTD_c_experimentalParam11 as ZSTD_c_blockDelimiters,
            ZSTD_c_experimentalParam12 as ZSTD_c_validateSequences,
            ZSTD_c_experimentalParam13 as ZSTD_c_useBlockSplitter,
            ZSTD_c_experimentalParam14 as ZSTD_c_useRowMatchFinder,
            ZSTD_c_experimentalParam15 as ZSTD_c_deterministicRefPrefix,
            ZSTD_c_experimentalParam16 as ZSTD_c_prefetchCDictTables,
            ZSTD_c_experimentalParam17 as ZSTD_c_enableSeqProducerFallback,
            ZSTD_c_experimentalParam18 as ZSTD_c_maxBlockSize,
            ZSTD_c_experimentalParam19 as ZSTD_c_searchForExternalRepcodes,
            ZSTD_c_experimentalParam2 as ZSTD_c_format,
            ZSTD_c_experimentalParam3 as ZSTD_c_forceMaxWindow,
            ZSTD_c_experimentalParam4 as ZSTD_c_forceAttachDict,
            ZSTD_c_experimentalParam5 as ZSTD_c_literalCompressionMode,
            ZSTD_c_experimentalParam7 as ZSTD_c_srcSizeHint,
            ZSTD_c_experimentalParam8 as ZSTD_c_enableDedicatedDictSearch,
            ZSTD_c_experimentalParam9 as ZSTD_c_stableInBuffer,
        };

        use zstd_sys::ZSTD_cParameter::*;
        use CParameter::*;

        let (param, value) = match param {
            #[cfg(feature = "experimental")]
            RSyncable(rsyncable) => (ZSTD_c_rsyncable, rsyncable as c_int),
            #[cfg(feature = "experimental")]
            Format(format) => (ZSTD_c_format, format as c_int),
            #[cfg(feature = "experimental")]
            ForceMaxWindow(force) => (ZSTD_c_forceMaxWindow, force as c_int),
            #[cfg(feature = "experimental")]
            ForceAttachDict(force) => (ZSTD_c_forceAttachDict, force as c_int),
            #[cfg(feature = "experimental")]
            LiteralCompressionMode(mode) => {
                (ZSTD_c_literalCompressionMode, mode as c_int)
            }
            #[cfg(feature = "experimental")]
            SrcSizeHint(value) => (ZSTD_c_srcSizeHint, value as c_int),
            #[cfg(feature = "experimental")]
            EnableDedicatedDictSearch(enable) => {
                (ZSTD_c_enableDedicatedDictSearch, enable as c_int)
            }
            // In stable mode zstd keeps reading from the `InBuffer` of one
            // call during `flush_stream`/`end_stream`, but the slice is only
            // borrowed for that one call: the safe API cannot uphold this.
            #[cfg(feature = "experimental")]
            StableInBuffer(true) => return Err(PARAMETER_UNSUPPORTED_ERROR),
            #[cfg(feature = "experimental")]
            StableInBuffer(false) => (ZSTD_c_stableInBuffer, 0),
            #[cfg(feature = "experimental")]
            StableOutBuffer(stable) => {
                (ZSTD_c_stableOutBuffer, stable as c_int)
            }
            #[cfg(feature = "experimental")]
            BlockDelimiters(value) => (ZSTD_c_blockDelimiters, value as c_int),
            #[cfg(feature = "experimental")]
            ValidateSequences(validate) => {
                (ZSTD_c_validateSequences, validate as c_int)
            }
            #[cfg(feature = "experimental")]
            UseBlockSplitter(split) => {
                (ZSTD_c_useBlockSplitter, split as c_int)
            }
            #[cfg(feature = "experimental")]
            UseRowMatchFinder(mode) => {
                (ZSTD_c_useRowMatchFinder, mode as c_int)
            }
            #[cfg(feature = "experimental")]
            DeterministicRefPrefix(deterministic) => {
                (ZSTD_c_deterministicRefPrefix, deterministic as c_int)
            }
            #[cfg(feature = "experimental")]
            PrefetchCDictTables(prefetch) => {
                (ZSTD_c_prefetchCDictTables, prefetch as c_int)
            }
            #[cfg(feature = "experimental")]
            EnableSeqProducerFallback(enable) => {
                (ZSTD_c_enableSeqProducerFallback, enable as c_int)
            }
            #[cfg(feature = "experimental")]
            MaxBlockSize(value) => (ZSTD_c_maxBlockSize, value as c_int),
            #[cfg(feature = "experimental")]
            SearchForExternalRepcodes(value) => {
                (ZSTD_c_searchForExternalRepcodes, value as c_int)
            }
            TargetCBlockSize(value) => {
                (ZSTD_c_targetCBlockSize, value as c_int)
            }
            CompressionLevel(level) => (ZSTD_c_compressionLevel, level),
            WindowLog(value) => (ZSTD_c_windowLog, value as c_int),
            HashLog(value) => (ZSTD_c_hashLog, value as c_int),
            ChainLog(value) => (ZSTD_c_chainLog, value as c_int),
            SearchLog(value) => (ZSTD_c_searchLog, value as c_int),
            MinMatch(value) => (ZSTD_c_minMatch, value as c_int),
            TargetLength(value) => (ZSTD_c_targetLength, value as c_int),
            Strategy(strategy) => (ZSTD_c_strategy, strategy as c_int),
            EnableLongDistanceMatching(flag) => {
                (ZSTD_c_enableLongDistanceMatching, flag as c_int)
            }
            LdmHashLog(value) => (ZSTD_c_ldmHashLog, value as c_int),
            LdmMinMatch(value) => (ZSTD_c_ldmMinMatch, value as c_int),
            LdmBucketSizeLog(value) => {
                (ZSTD_c_ldmBucketSizeLog, value as c_int)
            }
            LdmHashRateLog(value) => (ZSTD_c_ldmHashRateLog, value as c_int),
            ContentSizeFlag(flag) => (ZSTD_c_contentSizeFlag, flag as c_int),
            ChecksumFlag(flag) => (ZSTD_c_checksumFlag, flag as c_int),
            DictIdFlag(flag) => (ZSTD_c_dictIDFlag, flag as c_int),

            NbWorkers(value) => (ZSTD_c_nbWorkers, value as c_int),

            JobSize(value) => (ZSTD_c_jobSize, value as c_int),

            OverlapSizeLog(value) => (ZSTD_c_overlapLog, value as c_int),
        };

        // Safety: Just FFI
        let res = parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_setParameter(self.0.as_ptr(), param, value)
        });
        if res.is_ok() && matches!(param, ZSTD_c_nbWorkers) {
            self.3.requested = value != 0;
            self.3.ever |= value != 0;
        }
        #[cfg(feature = "experimental")]
        if res.is_ok() && matches!(param, ZSTD_c_stableOutBuffer) {
            self.3.stable_out = value != 0;
        }
        res
    }

    /// Guarantee that the input size will be this value.
    ///
    /// If given `None`, assumes the size is unknown.
    ///
    /// Unless explicitly disabled, this will cause the size to be written in the compressed frame
    /// header.
    ///
    /// If the actual data given to compress has a different size, an error will be returned.
    pub fn set_pledged_src_size(
        &mut self,
        pledged_src_size: Option<u64>,
    ) -> SafeResult {
        // Safety: Just FFI
        let res = parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_setPledgedSrcSize(
                self.0.as_ptr(),
                pledged_src_size.unwrap_or(CONTENTSIZE_UNKNOWN) as c_ulonglong,
            )
        });
        if res.is_ok() {
            // See `Workers::short`.
            self.3.pledged_short = matches!(
                pledged_src_size,
                Some(size) if size <= ZSTDMT_JOBSIZE_MIN as u64
            );
        }
        res
    }

    /// Fails if the tables of this context may not be the ones its applied
    /// parameters describe.
    ///
    /// Starting a multi-threaded frame overwrites the applied parameters with
    /// those of the workers, but leaves the context's own stage and workspace
    /// as the last single-threaded frame left them. If that frame was
    /// abandoned before its first block, zstd still accepts the calls that
    /// work on this context directly, and overruns a workspace that was sized
    /// for other parameters. The applied parameters cannot be queried, so
    /// refuse these calls as soon as a multi-threaded frame could have been
    /// started.
    #[cfg(feature = "experimental")]
    fn guard_own_tables(&self) -> Result<(), ErrorCode> {
        if self.3.ever {
            return Err(STAGE_WRONG_ERROR);
        }
        Ok(())
    }

    /// Creates a copy of this context.
    ///
    /// This only works before any data has been compressed. An error will be
    /// returned otherwise.
    /// A poisoned context cannot be cloned until its session is reset.
    /// A context on which `NbWorkers` was ever set to a non-zero value cannot
    /// be cloned anymore.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn try_clone(
        &self,
        pledged_src_size: Option<u64>,
    ) -> Result<Self, ErrorCode> {
        // An error may leave the C context undefined; do not copy it.
        self.2.guard()?;
        self.guard_own_tables()?;
        let context = Self::try_create().ok_or(MEMORY_ALLOCATION_ERROR)?;

        // Safety: Just FFI
        parse_code(unsafe {
            zstd_sys::ZSTD_copyCCtx(
                context.0.as_ptr(),
                self.0.as_ptr(),
                pledged_src_size.unwrap_or(CONTENTSIZE_UNKNOWN),
            )
        })?;

        Ok(context)
    }

    /// Wraps the `ZSTD_getBlockSize()` function.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn get_block_size(&self) -> usize {
        // Safety: Just FFI
        unsafe { zstd_sys::ZSTD_getBlockSize(self.0.as_ptr()) }
    }

    /// Wraps the `ZSTD_compressBlock()` function.
    ///
    /// Returns an error once `NbWorkers` was set to a non-zero value on this
    /// context.
    ///
    /// # Safety
    ///
    /// `src` becomes this context's history window, so it must stay allocated and unmodified until
    /// the next call to `compress_block` on this context (or until this context is dropped), as the
    /// following block is compressed against it.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub unsafe fn compress_block<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
    ) -> SafeResult {
        self.guard_own_tables()?;
        // Safety: ZSTD_compressBlock returns the number of bytes written.
        unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_compressBlock(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                ))
            })
        }
    }

    /// Returns the recommended input buffer size.
    ///
    /// Using this size may result in minor performance boost.
    pub fn in_size() -> usize {
        // Safety: Just FFI
        unsafe { zstd_sys::ZSTD_CStreamInSize() }
    }

    /// Returns the recommended output buffer size.
    ///
    /// Using this may result in minor performance boost.
    pub fn out_size() -> usize {
        // Safety: Just FFI
        unsafe { zstd_sys::ZSTD_CStreamOutSize() }
    }

    /// Use a shared thread pool for this context.
    ///
    /// Thread pool must outlive the context.
    #[cfg(all(feature = "experimental", feature = "zstdmt"))]
    #[cfg_attr(
        feature = "doc-cfg",
        doc(cfg(all(feature = "experimental", feature = "zstdmt")))
    )]
    pub fn ref_thread_pool<'b>(&mut self, pool: &'b ThreadPool) -> SafeResult
    where
        'b: 'a,
    {
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_refThreadPool(self.0.as_ptr(), pool.0.as_ptr())
        })
    }

    /// Return to using a private thread pool for this context.
    #[cfg(all(feature = "experimental", feature = "zstdmt"))]
    #[cfg_attr(
        feature = "doc-cfg",
        doc(cfg(all(feature = "experimental", feature = "zstdmt")))
    )]
    pub fn disable_thread_pool(&mut self) -> SafeResult {
        parse_code(unsafe {
            zstd_sys::ZSTD_CCtx_refThreadPool(
                self.0.as_ptr(),
                core::ptr::null_mut(),
            )
        })
    }

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn get_frame_progression(&self) -> FrameProgression {
        // Safety: Just FFI
        unsafe { zstd_sys::ZSTD_getFrameProgression(self.0.as_ptr()) }
    }
}

impl<'a> Drop for CCtx<'a> {
    fn drop(&mut self) {
        // zstd up to 1.5.7 frees the dictionary before it stops the worker
        // threads, and does not stop a shared pool's at all: wait for the
        // jobs of the frame in progress first.
        self.drain();
        // Safety: Just FFI
        unsafe {
            zstd_sys::ZSTD_freeCCtx(self.0.as_ptr());
        }
    }
}

unsafe impl Send for CCtx<'_> {}
// Non thread-safe methods already take `&mut self`, so it's fine to implement Sync here.
unsafe impl Sync for CCtx<'_> {}

unsafe fn c_char_to_str(text: *const c_char) -> &'static str {
    core::ffi::CStr::from_ptr(text)
        .to_str()
        .expect("bad error message from zstd")
}

/// Returns the error string associated with an error code.
pub fn get_error_name(code: usize) -> &'static str {
    unsafe {
        // Safety: assumes ZSTD returns a well-formed utf8 string.
        let name = zstd_sys::ZSTD_getErrorName(code);
        c_char_to_str(name)
    }
}

/// A Decompression Context.
///
/// The lifetime references the potential dictionary used for this context.
///
/// If no dictionary was used, it will most likely be `'static`.
///
/// Same as `DStream`.
pub struct DCtx<'a>(
    NonNull<zstd_sys::ZSTD_DCtx>,
    PhantomData<&'a ()>,
    Poison,
    DState,
);

/// What a `DCtx` remembers about its own use.
#[derive(Clone, Copy, Debug, Default)]
struct DState {
    /// `RefMultipleDDicts` was ever enabled, see `try_clone`.
    #[cfg(feature = "experimental")]
    multiple_ddicts: bool,
    /// A streaming (or block) call ran since the session was last reset, so
    /// a stream may be open, see `finish_one_shot`.
    streamed: bool,
}

impl Default for DCtx<'_> {
    fn default() -> Self {
        DCtx::create()
    }
}

impl<'a> DCtx<'a> {
    /// Try to create a new decompression context.
    ///
    /// Returns `None` if the operation failed (for example, not enough memory).
    pub fn try_create() -> Option<Self> {
        Some(DCtx(
            NonNull::new(unsafe { zstd_sys::ZSTD_createDCtx() })?,
            PhantomData,
            Poison::default(),
            DState::default(),
        ))
    }

    /// Creates a new decoding context.
    ///
    /// # Panics
    ///
    /// If the context creation fails.
    pub fn create() -> Self {
        Self::try_create()
            .expect("zstd returned null pointer when creating new context")
    }

    /// Ends the session of a one-shot decompression, and passes `res` through.
    ///
    /// zstd.h has these calls imply a reset, but they leave the stream stage
    /// alone. A stream they interrupted would then carry on from a state that
    /// still points at buffers and dictionaries borrowed for that one call. So
    /// reset explicitly: the next stream starts from scratch.
    ///
    /// Only an open stream can carry on, though: a new one starts by setting
    /// everything up again from the context's own dictionary. So when no
    /// streaming call ran since the last reset, there is nothing to end, and
    /// skipping the reset saves a call per frame - which shows on tiny ones.
    ///
    /// Only the check is inlined: it runs once per frame.
    #[inline]
    fn finish_one_shot(&mut self, res: SafeResult) -> SafeResult {
        if self.3.streamed || self.2.guard().is_err() {
            self.reset_after_one_shot(res)
        } else {
            res
        }
    }

    #[cold]
    #[inline(never)]
    fn reset_after_one_shot(&mut self, res: SafeResult) -> SafeResult {
        self.reset(ResetDirective::SessionOnly).and(res)
    }

    /// Fully decompress the given frame.
    ///
    /// This decompress an entire frame in-memory. If you can have enough memory to store both the
    /// input and output buffer, then it may be faster that streaming decompression.
    ///
    /// Wraps the `ZSTD_decompressDCtx()` function.
    pub fn decompress<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
    ) -> SafeResult {
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_decompressDCtx(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Fully decompress the given frame using a dictionary.
    ///
    /// Dictionary must be identical to the one used during compression.
    ///
    /// If you plan on using the same dictionary multiple times, it is faster to create a `DDict`
    /// first and use `decompress_using_ddict`.
    ///
    /// Wraps `ZSTD_decompress_usingDict`
    pub fn decompress_using_dict<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
        dict: &[u8],
    ) -> SafeResult {
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_decompress_usingDict(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                    ptr_void(dict),
                    dict.len(),
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Fully decompress the given frame using a dictionary.
    ///
    /// Dictionary must be identical to the one used during compression.
    ///
    /// Wraps the `ZSTD_decompress_usingDDict()` function.
    pub fn decompress_using_ddict<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
        ddict: &DDict<'_>,
    ) -> SafeResult {
        let res = unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_decompress_usingDDict(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                    ddict.0.as_ptr(),
                ))
            })
        };
        self.finish_one_shot(res)
    }

    /// Initializes an existing `DStream` for decompression.
    ///
    /// This is equivalent to calling:
    /// * `reset(SessionOnly)`
    /// * `disable_dictionary()`
    ///
    /// Wraps the `ZSTD_initCStream()` function.
    pub fn init(&mut self) -> SafeResult {
        self.2.clear();
        let code = unsafe { zstd_sys::ZSTD_initDStream(self.0.as_ptr()) };
        self.2.record(parse_code(code))
    }

    /// Wraps the `ZSTD_initDStream_usingDict()` function.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    #[deprecated]
    pub fn init_using_dict(&mut self, dict: &[u8]) -> SafeResult {
        self.2.clear();
        let code = unsafe {
            zstd_sys::ZSTD_initDStream_usingDict(
                self.0.as_ptr(),
                ptr_void(dict),
                dict.len(),
            )
        };
        parse_code(code)
    }

    /// Wraps the `ZSTD_initDStream_usingDDict()` function.
    ///
    /// Dictionary must outlive the context.
    ///
    /// ```compile_fail,E0515
    /// # #![allow(deprecated)]
    /// fn decompressor() -> zstd_safe::DCtx<'static> {
    ///     let ddict = zstd_safe::DDict::create(b"dictionary");
    ///     let mut dctx = zstd_safe::DCtx::create();
    ///     dctx.init_using_ddict(&ddict).unwrap();
    ///     dctx // `ddict` would be dropped while `dctx` still points at it.
    /// }
    /// ```
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    #[deprecated]
    pub fn init_using_ddict<'b>(&mut self, ddict: &'a DDict<'b>) -> SafeResult
    where
        'b: 'a,
    {
        self.2.clear();
        // Safety: Just FFI
        let code = unsafe {
            zstd_sys::ZSTD_initDStream_usingDDict(
                self.0.as_ptr(),
                ddict.0.as_ptr(),
            )
        };
        self.2.record(parse_code(code))
    }

    /// Resets the state of the context.
    ///
    /// Depending on the reset mode, it can reset the session, the parameters, or both.
    ///
    /// Wraps the `ZSTD_DCtx_reset()` function.
    pub fn reset(&mut self, reset: ResetDirective) -> SafeResult {
        let resets_session = reset.resets_session();
        let res = parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_reset(self.0.as_ptr(), reset.as_sys())
        });
        if res.is_ok() && resets_session {
            self.2.clear();
            self.3.streamed = false;
        }
        res
    }

    /// Loads a dictionary.
    ///
    /// This will let this context decompress frames that were compressed using this dictionary.
    ///
    /// The dictionary content will be copied internally and does not need to be kept alive after
    /// calling this function.
    ///
    /// If you need to use the same dictionary for multiple contexts, it may be more efficient to
    /// create a `DDict` first, then loads that.
    ///
    /// The dictionary will apply to all future frames, until a new dictionary is set.
    pub fn load_dictionary(&mut self, dict: &[u8]) -> SafeResult {
        parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_loadDictionary(
                self.0.as_ptr(),
                ptr_void(dict),
                dict.len(),
            )
        })
    }

    /// Return to "no-dictionary" mode.
    ///
    /// This will disable any dictionary/prefix previously registered for future frames.
    pub fn disable_dictionary(&mut self) -> SafeResult {
        parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_loadDictionary(
                self.0.as_ptr(),
                core::ptr::null(),
                0,
            )
        })
    }

    /// References a dictionary.
    ///
    /// This will let this context decompress frames compressed with the same dictionary.
    ///
    /// It will apply to all frames decompressed by this context (until a new dictionary is set).
    ///
    /// Wraps the `ZSTD_DCtx_refDDict()` function.
    pub fn ref_ddict<'b>(&mut self, ddict: &DDict<'b>) -> SafeResult
    where
        'b: 'a,
    {
        parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_refDDict(self.0.as_ptr(), ddict.0.as_ptr())
        })
    }

    /// Use some prefix as single-use dictionary for the next frame.
    ///
    /// Just like a dictionary, this only works if compression was done with the same prefix.
    ///
    /// But unlike a dictionary, this only applies to the next frame.
    ///
    /// Wraps the `ZSTD_DCtx_refPrefix()` function.
    pub fn ref_prefix<'b>(&mut self, prefix: &'b [u8]) -> SafeResult
    where
        'b: 'a,
    {
        parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_refPrefix(
                self.0.as_ptr(),
                ptr_void(prefix),
                prefix.len(),
            )
        })
    }

    /// Sets a decompression parameter.
    pub fn set_parameter(&mut self, param: DParameter) -> SafeResult {
        #[cfg(feature = "experimental")]
        use zstd_sys::ZSTD_dParameter::{
            ZSTD_d_experimentalParam1 as ZSTD_d_format,
            ZSTD_d_experimentalParam2 as ZSTD_d_stableOutBuffer,
            ZSTD_d_experimentalParam3 as ZSTD_d_forceIgnoreChecksum,
            ZSTD_d_experimentalParam4 as ZSTD_d_refMultipleDDicts,
        };

        use zstd_sys::ZSTD_dParameter::*;
        use DParameter::*;

        let (param, value) = match param {
            #[cfg(feature = "experimental")]
            Format(format) => (ZSTD_d_format, format as c_int),
            #[cfg(feature = "experimental")]
            StableOutBuffer(stable) => {
                (ZSTD_d_stableOutBuffer, stable as c_int)
            }
            #[cfg(feature = "experimental")]
            ForceIgnoreChecksum(force) => {
                (ZSTD_d_forceIgnoreChecksum, force as c_int)
            }
            #[cfg(feature = "experimental")]
            RefMultipleDDicts(value) => {
                // From now on `ref_ddict` may allocate the table of
                // dictionaries, which zstd keeps until the context is freed.
                self.3.multiple_ddicts |= value;
                (ZSTD_d_refMultipleDDicts, value as c_int)
            }

            WindowLogMax(value) => (ZSTD_d_windowLogMax, value as c_int),
        };

        parse_code(unsafe {
            zstd_sys::ZSTD_DCtx_setParameter(self.0.as_ptr(), param, value)
        })
    }

    /// Performs a step of a streaming decompression operation.
    ///
    /// This will read some data from `input` and/or write some data to `output`.
    ///
    /// # Returns
    ///
    /// * `Ok(0)` if the current frame just finished decompressing successfully.
    /// * `Ok(hint)` with a hint for the "ideal" amount of input data to provide in the next call.
    ///     Can be safely ignored.
    ///
    /// Wraps the `ZSTD_decompressStream()` function.
    pub fn decompress_stream<C: WriteBuf + ?Sized>(
        &mut self,
        output: &mut OutBuffer<'_, C>,
        input: &mut InBuffer<'_>,
    ) -> SafeResult {
        self.2.guard()?;
        self.3.streamed = true;
        let mut output = output.wrap();
        let mut input = input.wrap();
        let code = unsafe {
            zstd_sys::ZSTD_decompressStream(
                self.0.as_ptr(),
                ptr_mut(&mut output),
                ptr_mut(&mut input),
            )
        };
        let res = output.check(parse_code(code));
        self.2.record(input.check(res))
    }

    /// Wraps the `ZSTD_DStreamInSize()` function.
    ///
    /// Returns a hint for the recommended size of the input buffer for decompression.
    pub fn in_size() -> usize {
        unsafe { zstd_sys::ZSTD_DStreamInSize() }
    }

    /// Wraps the `ZSTD_DStreamOutSize()` function.
    ///
    /// Returns a hint for the recommended size of the output buffer for decompression.
    pub fn out_size() -> usize {
        unsafe { zstd_sys::ZSTD_DStreamOutSize() }
    }

    /// Wraps the `ZSTD_sizeof_DCtx()` function.
    pub fn sizeof(&self) -> usize {
        unsafe { zstd_sys::ZSTD_sizeof_DCtx(self.0.as_ptr()) }
    }

    /// Wraps the `ZSTD_decompressBlock()` function.
    ///
    /// # Safety
    ///
    /// The bytes written to `dst` become this context's history window, so `dst` must stay
    /// allocated and unmodified until the next call to `decompress_block` or `insert_block` on this
    /// context (or until this context is dropped), as the following block is decoded against it.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub unsafe fn decompress_block<C: WriteBuf + ?Sized>(
        &mut self,
        dst: &mut C,
        src: &[u8],
    ) -> SafeResult {
        self.3.streamed = true;
        unsafe {
            dst.write_from(|buffer, capacity| {
                parse_code(zstd_sys::ZSTD_decompressBlock(
                    self.0.as_ptr(),
                    buffer,
                    capacity,
                    ptr_void(src),
                    src.len(),
                ))
            })
        }
    }

    /// Wraps the `ZSTD_insertBlock()` function.
    ///
    /// # Safety
    ///
    /// `block` becomes this context's history window, so it must stay allocated and unmodified
    /// until the next call to `decompress_block` or `insert_block` on this context (or until this
    /// context is dropped), as the following block is decoded against it.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub unsafe fn insert_block(&mut self, block: &[u8]) -> usize {
        self.3.streamed = true;
        unsafe {
            zstd_sys::ZSTD_insertBlock(
                self.0.as_ptr(),
                ptr_void(block),
                block.len(),
            )
        }
    }

    /// Creates a copy of this context.
    ///
    /// This only works before any data has been decompressed. An error will be
    /// returned otherwise.
    /// A poisoned context cannot be cloned until its session is reset.
    /// The copy always starts at the beginning of a frame.
    ///
    /// A dictionary this context merely references (see [`DCtx::ref_ddict`])
    /// is referenced by the copy too. One it owns cannot be shared, so an
    /// error is also returned while a dictionary from
    /// [`DCtx::load_dictionary`] or a prefix from [`DCtx::ref_prefix`] is set,
    /// and once [`DParameter::RefMultipleDDicts`] has been enabled.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn try_clone(&self) -> Result<Self, ErrorCode> {
        use zstd_sys::ZSTD_dParameter::{
            ZSTD_d_experimentalParam2 as ZSTD_d_stableOutBuffer,
            ZSTD_d_windowLogMax,
        };

        // An error may leave the C context undefined; do not copy it.
        self.2.guard()?;
        let mut context = Self::try_create().ok_or(MEMORY_ALLOCATION_ERROR)?;

        // `ZSTD_copyDCtx()` is a plain `memcpy`, so a dictionary `self` owns
        // would end up owned, and freed, by both contexts. A loaded dictionary
        // or prefix shows up as memory a new context does not use - as do the
        // streaming buffers, once data has been decompressed. The table behind
        // `RefMultipleDDicts` is not counted there, and outlives the
        // parameter, hence the flag.
        if self.3.multiple_ddicts || self.sizeof() != context.sizeof() {
            return Err(STAGE_WRONG_ERROR);
        }

        // `ZSTD_copyDCtx` stops where the streaming buffers start and leaves
        // the parameters stored past that point (the window limit and the
        // output buffer mode) at their defaults. Carry them over explicitly,
        // on the fresh context so that it is still in its initial stage; the
        // copy below does not touch these fields.
        // Only for values that differ: the window limit reads back as a log,
        // which rounds zstd's default limit down.
        let get = |dctx: &Self, param| {
            let mut value: c_int = 0;
            // Safety: Just FFI
            parse_code(unsafe {
                zstd_sys::ZSTD_DCtx_getParameter(
                    dctx.0.as_ptr(),
                    param,
                    &mut value,
                )
            })
            .map(|_| value)
        };
        for param in [ZSTD_d_windowLogMax, ZSTD_d_stableOutBuffer] {
            let value = get(self, param)?;
            if value == get(&context, param)? {
                continue;
            }
            // Safety: Just FFI
            parse_code(unsafe {
                zstd_sys::ZSTD_DCtx_setParameter(
                    context.0.as_ptr(),
                    param,
                    value,
                )
            })?;
        }

        // Safety: Just FFI
        unsafe {
            zstd_sys::ZSTD_copyDCtx(context.0.as_ptr(), self.0.as_ptr())
        };

        // The position within a frame is copied, but not the state that goes
        // with it (a partial header, a legacy context), so start from a frame
        // boundary rather than run on what the new context never initialized.
        context.reset(ResetDirective::SessionOnly)?;

        Ok(context)
    }
}

impl Drop for DCtx<'_> {
    fn drop(&mut self) {
        unsafe {
            zstd_sys::ZSTD_freeDCtx(self.0.as_ptr());
        }
    }
}

unsafe impl Send for DCtx<'_> {}
// Non thread-safe methods already take `&mut self`, so it's fine to implement Sync here.
unsafe impl Sync for DCtx<'_> {}

/// Compression dictionary.
pub struct CDict<'a>(NonNull<zstd_sys::ZSTD_CDict>, PhantomData<&'a ()>);

impl CDict<'static> {
    /// Prepare a dictionary to compress data.
    ///
    /// This will make it easier for compression contexts to load this dictionary.
    ///
    /// The dictionary content will be copied internally, and does not need to be kept around.
    ///
    /// # Panics
    ///
    /// If loading this dictionary failed.
    pub fn create(
        dict_buffer: &[u8],
        compression_level: CompressionLevel,
    ) -> Self {
        Self::try_create(dict_buffer, compression_level)
            .expect("zstd returned null pointer when creating dict")
    }

    /// Prepare a dictionary to compress data.
    ///
    /// This will make it easier for compression contexts to load this dictionary.
    ///
    /// The dictionary content will be copied internally, and does not need to be kept around.
    pub fn try_create(
        dict_buffer: &[u8],
        compression_level: CompressionLevel,
    ) -> Option<Self> {
        Some(CDict(
            NonNull::new(unsafe {
                zstd_sys::ZSTD_createCDict(
                    ptr_void(dict_buffer),
                    dict_buffer.len(),
                    compression_level,
                )
            })?,
            PhantomData,
        ))
    }
}

impl<'a> CDict<'a> {
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn create_by_reference(
        dict_buffer: &'a [u8],
        compression_level: CompressionLevel,
    ) -> Self {
        CDict(
            NonNull::new(unsafe {
                zstd_sys::ZSTD_createCDict_byReference(
                    ptr_void(dict_buffer),
                    dict_buffer.len(),
                    compression_level,
                )
            })
            .expect("zstd returned null pointer"),
            PhantomData,
        )
    }

    /// Returns the _current_ memory usage of this dictionary.
    ///
    /// Note that this may change over time.
    pub fn sizeof(&self) -> usize {
        unsafe { zstd_sys::ZSTD_sizeof_CDict(self.0.as_ptr()) }
    }

    /// Returns the dictionary ID for this dict.
    ///
    /// Returns `None` if this dictionary is empty or invalid.
    pub fn get_dict_id(&self) -> Option<NonZeroU32> {
        NonZeroU32::new(unsafe {
            zstd_sys::ZSTD_getDictID_fromCDict(self.0.as_ptr()) as u32
        })
    }
}

/// Wraps the `ZSTD_createCDict()` function.
pub fn create_cdict(
    dict_buffer: &[u8],
    compression_level: CompressionLevel,
) -> CDict<'static> {
    CDict::create(dict_buffer, compression_level)
}

impl<'a> Drop for CDict<'a> {
    fn drop(&mut self) {
        unsafe {
            zstd_sys::ZSTD_freeCDict(self.0.as_ptr());
        }
    }
}

unsafe impl<'a> Send for CDict<'a> {}
unsafe impl<'a> Sync for CDict<'a> {}

/// Wraps the `ZSTD_compress_usingCDict()` function.
pub fn compress_using_cdict(
    cctx: &mut CCtx<'_>,
    dst: &mut [u8],
    src: &[u8],
    cdict: &CDict<'_>,
) -> SafeResult {
    cctx.compress_using_cdict(dst, src, cdict)
}

/// A digested decompression dictionary.
pub struct DDict<'a>(NonNull<zstd_sys::ZSTD_DDict>, PhantomData<&'a ()>);

impl DDict<'static> {
    pub fn create(dict_buffer: &[u8]) -> Self {
        Self::try_create(dict_buffer)
            .expect("zstd returned null pointer when creating dict")
    }

    pub fn try_create(dict_buffer: &[u8]) -> Option<Self> {
        Some(DDict(
            NonNull::new(unsafe {
                zstd_sys::ZSTD_createDDict(
                    ptr_void(dict_buffer),
                    dict_buffer.len(),
                )
            })?,
            PhantomData,
        ))
    }
}

impl<'a> DDict<'a> {
    pub fn sizeof(&self) -> usize {
        unsafe { zstd_sys::ZSTD_sizeof_DDict(self.0.as_ptr()) }
    }

    /// Wraps the `ZSTD_createDDict_byReference()` function.
    ///
    /// The dictionary will keep referencing `dict_buffer`.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    pub fn create_by_reference(dict_buffer: &'a [u8]) -> Self {
        DDict(
            NonNull::new(unsafe {
                zstd_sys::ZSTD_createDDict_byReference(
                    ptr_void(dict_buffer),
                    dict_buffer.len(),
                )
            })
            .expect("zstd returned null pointer"),
            PhantomData,
        )
    }

    /// Returns the dictionary ID for this dict.
    ///
    /// Returns `None` if this dictionary is empty or invalid.
    pub fn get_dict_id(&self) -> Option<NonZeroU32> {
        NonZeroU32::new(unsafe {
            zstd_sys::ZSTD_getDictID_fromDDict(self.0.as_ptr()) as u32
        })
    }
}

/// Wraps the `ZSTD_createDDict()` function.
///
/// It copies the dictionary internally, so the resulting `DDict` is `'static`.
pub fn create_ddict(dict_buffer: &[u8]) -> DDict<'static> {
    DDict::create(dict_buffer)
}

impl<'a> Drop for DDict<'a> {
    fn drop(&mut self) {
        unsafe {
            zstd_sys::ZSTD_freeDDict(self.0.as_ptr());
        }
    }
}

unsafe impl<'a> Send for DDict<'a> {}
unsafe impl<'a> Sync for DDict<'a> {}

/// A shared thread pool for one or more compression contexts
#[cfg(all(feature = "experimental", feature = "zstdmt"))]
#[cfg_attr(
    feature = "doc-cfg",
    doc(cfg(all(feature = "experimental", feature = "zstdmt")))
)]
pub struct ThreadPool(NonNull<zstd_sys::ZSTD_threadPool>);

#[cfg(all(feature = "experimental", feature = "zstdmt"))]
#[cfg_attr(
    feature = "doc-cfg",
    doc(cfg(all(feature = "experimental", feature = "zstdmt")))
)]
impl ThreadPool {
    /// Create a thread pool with the specified number of threads.
    ///
    /// # Panics
    ///
    /// If creating the thread pool failed.
    pub fn new(num_threads: usize) -> Self {
        Self::try_new(num_threads)
            .expect("zstd returned null pointer when creating thread pool")
    }

    /// Create a thread pool with the specified number of threads.
    pub fn try_new(num_threads: usize) -> Option<Self> {
        Some(Self(NonNull::new(unsafe {
            zstd_sys::ZSTD_createThreadPool(num_threads)
        })?))
    }
}

#[cfg(all(feature = "experimental", feature = "zstdmt"))]
#[cfg_attr(
    feature = "doc-cfg",
    doc(cfg(all(feature = "experimental", feature = "zstdmt")))
)]
impl Drop for ThreadPool {
    fn drop(&mut self) {
        unsafe {
            zstd_sys::ZSTD_freeThreadPool(self.0.as_ptr());
        }
    }
}

#[cfg(all(feature = "experimental", feature = "zstdmt"))]
#[cfg_attr(
    feature = "doc-cfg",
    doc(cfg(all(feature = "experimental", feature = "zstdmt")))
)]
unsafe impl Send for ThreadPool {}
#[cfg(all(feature = "experimental", feature = "zstdmt"))]
#[cfg_attr(
    feature = "doc-cfg",
    doc(cfg(all(feature = "experimental", feature = "zstdmt")))
)]
unsafe impl Sync for ThreadPool {}

/// Wraps the `ZSTD_decompress_usingDDict()` function.
pub fn decompress_using_ddict(
    dctx: &mut DCtx<'_>,
    dst: &mut [u8],
    src: &[u8],
    ddict: &DDict<'_>,
) -> SafeResult {
    dctx.decompress_using_ddict(dst, src, ddict)
}

/// Compression stream.
///
/// Same as `CCtx`.
pub type CStream<'a> = CCtx<'a>;

// CStream can't be shared across threads, so it does not implement Sync.

/// Allocates a new `CStream`.
pub fn create_cstream<'a>() -> CStream<'a> {
    CCtx::create()
}

/// Prepares an existing `CStream` for compression at the given level.
pub fn init_cstream(
    zcs: &mut CStream<'_>,
    compression_level: CompressionLevel,
) -> SafeResult {
    zcs.init(compression_level)
}

#[derive(Debug)]
/// Wrapper around an input buffer.
///
/// Bytes will be read starting at `src[pos]`.
///
/// `pos` will be updated after reading.
///
/// # Invariant
///
/// `pos <= src.len()`
///
/// Both fields are public, so this is checked (and panics if it does not hold)
/// whenever the buffer is given to zstd.
pub struct InBuffer<'a> {
    pub src: &'a [u8],
    pub pos: usize,
}

/// Describe a bytes container, like `Vec<u8>`.
///
/// Represents a contiguous segment of allocated memory, a prefix of which is initialized.
///
/// It allows starting from an uninitializes chunk of memory and writing to it, progressively
/// initializing it. No re-allocation typically occur after the initial creation.
///
/// The main implementors are:
/// * `Vec<u8>` and similar structures. These hold both a length (initialized data) and a capacity
///   (allocated memory).
///
///   Use `Vec::with_capacity` to create an empty `Vec` with non-zero capacity, and the length
///   field will be updated to cover the data written to it (as long as it fits in the given
///   capacity).
/// * `[u8]` and `[u8; N]`. These must start already-initialized, and will not be resized. It will
///   be up to the caller to only use the part that was written (as returned by the various writing
///   operations).
/// * `std::io::Cursor<T: WriteBuf>`. This will ignore data before the cursor's position, and
///   append data after that.
pub unsafe trait WriteBuf {
    /// Returns the valid data part of this container. Should only cover initialized data.
    fn as_slice(&self) -> &[u8];

    /// Returns the full capacity of this container. May include uninitialized data.
    fn capacity(&self) -> usize;

    /// Returns a pointer to the start of the data.
    fn as_mut_ptr(&mut self) -> *mut u8;

    /// Indicates that the first `n` bytes of the container have been written.
    ///
    /// Safety: this should only be called if the `n` first bytes of this buffer have actually been
    /// initialized.
    unsafe fn filled_until(&mut self, n: usize);

    /// Call the given closure using the pointer and capacity from `self`.
    ///
    /// Assumes the given function returns a parseable code, which if valid, represents how many
    /// bytes were written to `self`.
    ///
    /// The given closure must treat its first argument as pointing to potentially uninitialized
    /// memory, and should not read from it.
    ///
    /// In addition, it must have written at least `n` bytes contiguously from this pointer, where
    /// `n` is the returned value.
    unsafe fn write_from<F>(&mut self, f: F) -> SafeResult
    where
        F: FnOnce(*mut c_void, usize) -> SafeResult,
    {
        let capacity = self.capacity();
        let res = f(ptr_mut_void(self), capacity);
        if let Ok(n) = res {
            // Nothing can have been written past the end of the buffer; a
            // larger count would mark memory we do not own as initialized.
            assert!(n <= capacity, "zstd reported writing past the buffer");
            self.filled_until(n);
        }
        res
    }
}

/// The position of a `Cursor`, as an index into the buffer it wraps.
///
/// `Cursor` stores the position as a `u64` and lets it be set anywhere, so on a
/// target where `usize` is narrower it can name an offset no buffer can hold.
/// Saturating leaves such a position out of range, where the bounds checks
/// below reject it; casting would wrap it around into a valid-looking offset
/// and quietly read or write the wrong part of the buffer.
#[cfg(feature = "std")]
fn cursor_position<T>(cursor: &std::io::Cursor<T>) -> usize {
    use core::convert::TryFrom;

    usize::try_from(cursor.position()).unwrap_or(usize::MAX)
}

#[cfg(feature = "std")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "std")))]
unsafe impl<T> WriteBuf for std::io::Cursor<T>
where
    T: WriteBuf,
{
    fn as_slice(&self) -> &[u8] {
        // A cursor past the inner buffer's initialized part has nothing
        // initialized after it yet (see `filled_until` below).
        self.get_ref()
            .as_slice()
            .get(cursor_position(self)..)
            .unwrap_or(&[])
    }

    fn capacity(&self) -> usize {
        self.get_ref()
            .capacity()
            .saturating_sub(cursor_position(self))
    }

    fn as_mut_ptr(&mut self) -> *mut u8 {
        let start = cursor_position(self);
        assert!(start <= self.get_ref().capacity());
        // Safety: start is still in the same memory allocation
        unsafe { self.get_mut().as_mut_ptr().add(start) }
    }

    unsafe fn filled_until(&mut self, n: usize) {
        // Here we assume data _before_ self.position() was already initialized.
        // Egh it's not actually guaranteed by Cursor? So let's guarantee it ourselves.
        // Since the cursor wraps another `WriteBuf`, we know how much data is initialized there.
        let position = cursor_position(self);
        let capacity = self.get_ref().capacity();
        if n == 0 && position > capacity {
            // Nothing was written, and nothing could have been: there is
            // no room at all after the cursor.
            return;
        }
        // Otherwise the caller wrote `n` bytes starting at `position`, so
        // `position` is inside the buffer. Checking it before the zero-fill
        // below keeps a position that could not be converted from running off
        // the end.
        //
        // Note that `n == 0` still truncates the data after the cursor, like
        // any other `n` does: an empty result must not leave a previous one
        // in place.
        assert!(position <= capacity);
        let initialized = self.get_ref().as_slice().len();
        if let Some(uninitialized) = position.checked_sub(initialized) {
            // Here, the cursor is further than the known-initialized part.
            // Cursor's solution is to pad with zeroes, so let's do the same.
            // We'll zero bytes from the end of valid data (as_slice().len()) to the cursor position.

            // Safety:
            // * We checked `position <= self.get_ref().capacity()`
            // * This means that `position` is within (or one past) the nested
            //   pointer's allocation.
            // * Finally, `initialized + uninitialized = position`, so the entire byte
            //   range here is within the allocation
            unsafe {
                self.get_mut()
                    .as_mut_ptr()
                    .add(initialized)
                    .write_bytes(0u8, uninitialized)
            };
        }

        let start = position;
        assert!(start + n <= self.get_ref().capacity());
        self.get_mut().filled_until(start + n);
    }
}

#[cfg(feature = "std")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "std")))]
unsafe impl<'a> WriteBuf for &'a mut std::vec::Vec<u8> {
    fn as_slice(&self) -> &[u8] {
        std::vec::Vec::as_slice(self)
    }

    fn capacity(&self) -> usize {
        std::vec::Vec::capacity(self)
    }

    fn as_mut_ptr(&mut self) -> *mut u8 {
        std::vec::Vec::as_mut_ptr(self)
    }

    unsafe fn filled_until(&mut self, n: usize) {
        std::vec::Vec::set_len(self, n)
    }
}

#[cfg(feature = "std")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "std")))]
unsafe impl WriteBuf for std::vec::Vec<u8> {
    fn as_slice(&self) -> &[u8] {
        &self[..]
    }
    fn capacity(&self) -> usize {
        self.capacity()
    }
    fn as_mut_ptr(&mut self) -> *mut u8 {
        self.as_mut_ptr()
    }
    unsafe fn filled_until(&mut self, n: usize) {
        self.set_len(n);
    }
}

#[cfg(feature = "arrays")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "arrays")))]
unsafe impl<const N: usize> WriteBuf for [u8; N] {
    fn as_slice(&self) -> &[u8] {
        self
    }
    fn capacity(&self) -> usize {
        self.len()
    }

    fn as_mut_ptr(&mut self) -> *mut u8 {
        (&mut self[..]).as_mut_ptr()
    }

    unsafe fn filled_until(&mut self, _n: usize) {
        // Assume the slice is already initialized
    }
}

unsafe impl WriteBuf for [u8] {
    fn as_slice(&self) -> &[u8] {
        self
    }
    fn capacity(&self) -> usize {
        self.len()
    }

    fn as_mut_ptr(&mut self) -> *mut u8 {
        self.as_mut_ptr()
    }

    unsafe fn filled_until(&mut self, _n: usize) {
        // Assume the slice is already initialized
    }
}

/*
// This is possible, but... why?
unsafe impl<'a> WriteBuf for OutBuffer<'a, [u8]> {
    fn as_slice(&self) -> &[u8] {
        self.dst
    }
    fn capacity(&self) -> usize {
        self.dst.len()
    }
    fn as_mut_ptr(&mut self) -> *mut u8 {
        self.dst.as_mut_ptr()
    }
    unsafe fn filled_until(&mut self, n: usize) {
        self.pos = n;
    }
}
*/

#[derive(Debug)]
/// Wrapper around an output buffer.
///
/// `C` is usually either `[u8]` or `Vec<u8>`.
///
/// Bytes will be written starting at `dst[pos]`.
///
/// `pos` will be updated after writing.
///
/// # Invariant
///
/// `pos <= dst.capacity()`
pub struct OutBuffer<'a, C: WriteBuf + ?Sized> {
    dst: &'a mut C,
    pos: usize,
}

/// Convenience method to get a mut pointer from a mut ref.
fn ptr_mut<B>(ptr_void: &mut B) -> *mut B {
    ptr_void as *mut B
}

/// Interface between a C-level ZSTD_outBuffer and a rust-level `OutBuffer`.
///
/// Will update the parent buffer from the C buffer on drop.
struct OutBufferWrapper<'a, 'b, C: WriteBuf + ?Sized> {
    buf: zstd_sys::ZSTD_outBuffer,
    parent: &'a mut OutBuffer<'b, C>,
}

impl<'a, 'b: 'a, C: WriteBuf + ?Sized> Deref for OutBufferWrapper<'a, 'b, C> {
    type Target = zstd_sys::ZSTD_outBuffer;

    fn deref(&self) -> &Self::Target {
        &self.buf
    }
}

impl<'a, 'b: 'a, C: WriteBuf + ?Sized> DerefMut
    for OutBufferWrapper<'a, 'b, C>
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.buf
    }
}

impl<'a, C: WriteBuf + ?Sized> OutBuffer<'a, C> {
    /// Returns a new `OutBuffer` around the given slice.
    ///
    /// Starts with `pos = 0`.
    pub fn around(dst: &'a mut C) -> Self {
        OutBuffer { dst, pos: 0 }
    }

    /// Returns a new `OutBuffer` around the given slice, starting at the given position.
    ///
    /// If `pos` is past the initialized part of `dst` (for example a `Vec`
    /// whose `len()` is smaller than `pos`), the gap is zero-filled, as a
    /// `Cursor` would do.
    ///
    /// # Panics
    ///
    /// If `pos > dst.capacity()`.
    pub fn around_pos(dst: &'a mut C, pos: usize) -> Self {
        if pos > dst.capacity() {
            panic!("Given position outside of the buffer bounds.");
        }

        let initialized = dst.as_slice().len();
        if pos > initialized {
            // Writing to this buffer marks everything before `pos` as
            // initialized (see `set_pos`), so it has to be before we start.
            // Safety: `initialized..pos` lies within `capacity()`, which
            // `WriteBuf` promises is valid to write to.
            unsafe {
                dst.as_mut_ptr()
                    .add(initialized)
                    .write_bytes(0u8, pos - initialized);
                dst.filled_until(pos);
            }
        }

        OutBuffer { dst, pos }
    }

    /// Returns the current cursor position.
    ///
    /// Guaranteed to be <= self.capacity()
    pub fn pos(&self) -> usize {
        assert!(self.pos <= self.dst.capacity());
        self.pos
    }

    /// Returns the capacity of the underlying buffer.
    pub fn capacity(&self) -> usize {
        self.dst.capacity()
    }

    /// Sets the new cursor position.
    ///
    /// # Panics
    ///
    /// If `pos > self.dst.capacity()`.
    ///
    /// # Safety
    ///
    /// Data up to `pos` must have actually been written to.
    pub unsafe fn set_pos(&mut self, pos: usize) {
        if pos > self.dst.capacity() {
            panic!("Given position outside of the buffer bounds.");
        }

        self.dst.filled_until(pos);

        self.pos = pos;
    }

    fn wrap<'b>(&'b mut self) -> OutBufferWrapper<'b, 'a, C> {
        OutBufferWrapper {
            buf: zstd_sys::ZSTD_outBuffer {
                dst: ptr_mut_void(self.dst),
                size: self.dst.capacity(),
                pos: self.pos,
            },
            parent: self,
        }
    }

    /// Returns the part of this buffer that was written to.
    pub fn as_slice<'b>(&'b self) -> &'a [u8]
    where
        'b: 'a,
    {
        let pos = self.pos;
        &self.dst.as_slice()[..pos]
    }

    /// Returns a pointer to the start of this buffer.
    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.dst.as_mut_ptr()
    }
}

impl<'a, 'b, C: WriteBuf + ?Sized> OutBufferWrapper<'a, 'b, C> {
    /// Passes `res` through, and forgets the position reported by zstd if it
    /// is an error, so the parent keeps the one from before the call.
    ///
    /// zstd.h makes no promise about `pos` after a failed call, and the legacy
    /// decoders report the whole output buffer as written when they fail; only
    /// a successful call vouches for the bytes up to `pos` being initialized.
    fn check(&mut self, res: SafeResult) -> SafeResult {
        if res.is_err() {
            self.buf.pos = self.parent.pos;
        }
        res
    }
}

impl<'a, 'b, C: WriteBuf + ?Sized> Drop for OutBufferWrapper<'a, 'b, C> {
    fn drop(&mut self) {
        // Safe because `self.buf.pos` either comes from a successful call,
        // which guarantees that data until there has been written, or was put
        // back to the parent's position by `check()`.
        unsafe { self.parent.set_pos(self.buf.pos) };
    }
}

struct InBufferWrapper<'a, 'b> {
    buf: zstd_sys::ZSTD_inBuffer,
    parent: &'a mut InBuffer<'b>,
}

impl<'a, 'b: 'a> Deref for InBufferWrapper<'a, 'b> {
    type Target = zstd_sys::ZSTD_inBuffer;

    fn deref(&self) -> &Self::Target {
        &self.buf
    }
}

impl<'a, 'b: 'a> DerefMut for InBufferWrapper<'a, 'b> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.buf
    }
}

impl<'a> InBuffer<'a> {
    /// Returns a new `InBuffer` around the given slice.
    ///
    /// Starts with `pos = 0`.
    pub fn around(src: &'a [u8]) -> Self {
        InBuffer { src, pos: 0 }
    }

    /// Returns the current cursor position.
    pub fn pos(&self) -> usize {
        self.pos
    }

    /// Sets the new cursor position.
    ///
    /// # Panics
    ///
    /// If `pos > self.src.len()`.
    pub fn set_pos(&mut self, pos: usize) {
        if pos > self.src.len() {
            panic!("Given position outside of the buffer bounds.");
        }
        self.pos = pos;
    }

    fn wrap<'b>(&'b mut self) -> InBufferWrapper<'b, 'a> {
        // The fields are public, so `pos` may not have gone through `set_pos`.
        // Not all zstd functions check it before reading from `src + pos`.
        if self.pos > self.src.len() {
            panic!("Given position outside of the buffer bounds.");
        }

        InBufferWrapper {
            buf: zstd_sys::ZSTD_inBuffer {
                src: ptr_void(self.src),
                size: self.src.len(),
                pos: self.pos,
            },
            parent: self,
        }
    }
}

impl<'a, 'b> InBufferWrapper<'a, 'b> {
    /// Passes `res` through, and forgets the position reported by zstd if it
    /// is an error, so the parent keeps the one from before the call.
    ///
    /// See `OutBufferWrapper::check()`: a failed call consumed nothing usable,
    /// and the legacy decoders report the whole input as read.
    fn check(&mut self, res: SafeResult) -> SafeResult {
        if res.is_err() {
            self.buf.pos = self.parent.pos;
        }
        res
    }
}

impl<'a, 'b> Drop for InBufferWrapper<'a, 'b> {
    fn drop(&mut self) {
        self.parent.set_pos(self.buf.pos);
    }
}

/// A Decompression stream.
///
/// Same as `DCtx`.
pub type DStream<'a> = DCtx<'a>;

// Some functions work on a "frame prefix".
// TODO: Define `struct FramePrefix(&[u8]);` and move these functions to it?
//
// Some other functions work on a dictionary (not CDict or DDict).
// Same thing?

/// Wraps the `ZSTD_findFrameCompressedSize()` function.
///
/// `src` should contain at least an entire frame.
pub fn find_frame_compressed_size(src: &[u8]) -> SafeResult {
    let code = unsafe {
        zstd_sys::ZSTD_findFrameCompressedSize(ptr_void(src), src.len())
    };
    parse_code(code)
}

/// Wraps the `ZSTD_getFrameContentSize()` function.
///
/// Args:
/// * `src`: A prefix of the compressed frame. It should at least include the frame header.
///
/// Returns:
/// * `Err(ContentSizeError)` if `src` is too small of a prefix, or if it appears corrupted.
/// * `Ok(None)` if the frame does not include a content size.
/// * `Ok(Some(content_size_in_bytes))` otherwise.
pub fn get_frame_content_size(
    src: &[u8],
) -> Result<Option<u64>, ContentSizeError> {
    parse_content_size(unsafe {
        zstd_sys::ZSTD_getFrameContentSize(ptr_void(src), src.len())
    })
}

/// Wraps the `ZSTD_findDecompressedSize()` function.
///
/// `src` should be exactly a sequence of ZSTD frames.
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn find_decompressed_size(
    src: &[u8],
) -> Result<Option<u64>, ContentSizeError> {
    parse_content_size(unsafe {
        zstd_sys::ZSTD_findDecompressedSize(ptr_void(src), src.len())
    })
}

/// Wraps the `ZSTD_isFrame()` function.
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn is_frame(buffer: &[u8]) -> bool {
    unsafe { zstd_sys::ZSTD_isFrame(ptr_void(buffer), buffer.len()) > 0 }
}

/// Wraps the `ZSTD_getDictID_fromDict()` function.
///
/// Returns `None` if the dictionary is not a valid zstd dictionary.
pub fn get_dict_id_from_dict(dict: &[u8]) -> Option<NonZeroU32> {
    NonZeroU32::new(unsafe {
        zstd_sys::ZSTD_getDictID_fromDict(ptr_void(dict), dict.len()) as u32
    })
}

/// Wraps the `ZSTD_getDictID_fromFrame()` function.
///
/// Returns `None` if the dictionary ID could not be decoded. This may happen if:
/// * The frame was not encoded with a dictionary.
/// * The frame intentionally did not include dictionary ID.
/// * The dictionary was non-conformant.
/// * `src` is too small and does not include the frame header.
/// * `src` is not a valid zstd frame prefix.
pub fn get_dict_id_from_frame(src: &[u8]) -> Option<NonZeroU32> {
    NonZeroU32::new(unsafe {
        zstd_sys::ZSTD_getDictID_fromFrame(ptr_void(src), src.len()) as u32
    })
}

/// What kind of context reset should be applied.
pub enum ResetDirective {
    /// Only the session will be reset.
    ///
    /// All parameters will be preserved (including the dictionary).
    /// But any frame being processed will be dropped.
    ///
    /// It can be useful to start re-using a context after an error or when an
    /// ongoing compression is no longer needed.
    SessionOnly,

    /// Only reset parameters (including dictionary or referenced prefix).
    ///
    /// All parameters will be reset to default values.
    ///
    /// This can only be done between sessions - no compression or decompression must be ongoing.
    Parameters,

    /// Reset both the session and parameters.
    ///
    /// The result is similar to a newly created context.
    SessionAndParameters,
}

impl ResetDirective {
    /// Does this drop a session in progress?
    ///
    /// Only a session reset brings a context back from the undefined state an
    /// error can leave it in - `Parameters` alone is refused while a session is
    /// open.
    fn resets_session(&self) -> bool {
        matches!(
            self,
            ResetDirective::SessionOnly | ResetDirective::SessionAndParameters
        )
    }

    fn as_sys(self) -> zstd_sys::ZSTD_ResetDirective {
        match self {
            ResetDirective::SessionOnly => zstd_sys::ZSTD_ResetDirective::ZSTD_reset_session_only,
            ResetDirective::Parameters => zstd_sys::ZSTD_ResetDirective::ZSTD_reset_parameters,
            ResetDirective::SessionAndParameters => zstd_sys::ZSTD_ResetDirective::ZSTD_reset_session_and_parameters,
        }
    }
}

#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum FrameFormat {
    /// Regular zstd format.
    One = zstd_sys::ZSTD_format_e::ZSTD_f_zstd1 as u32,

    /// Skip the 4 bytes identifying the content as zstd-compressed data.
    Magicless = zstd_sys::ZSTD_format_e::ZSTD_f_zstd1_magicless as u32,
}

#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum DictAttachPref {
    DefaultAttach =
        zstd_sys::ZSTD_dictAttachPref_e::ZSTD_dictDefaultAttach as u32,
    ForceAttach = zstd_sys::ZSTD_dictAttachPref_e::ZSTD_dictForceAttach as u32,
    ForceCopy = zstd_sys::ZSTD_dictAttachPref_e::ZSTD_dictForceCopy as u32,
    ForceLoad = zstd_sys::ZSTD_dictAttachPref_e::ZSTD_dictForceLoad as u32,
}

#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum ParamSwitch {
    Auto = zstd_sys::ZSTD_ParamSwitch_e::ZSTD_ps_auto as u32,
    Enable = zstd_sys::ZSTD_ParamSwitch_e::ZSTD_ps_enable as u32,
    Disable = zstd_sys::ZSTD_ParamSwitch_e::ZSTD_ps_disable as u32,
}

/// A compression parameter.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum CParameter {
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    RSyncable(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    Format(FrameFormat),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    ForceMaxWindow(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    ForceAttachDict(DictAttachPref),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    LiteralCompressionMode(ParamSwitch),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    SrcSizeHint(u32),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    EnableDedicatedDictSearch(bool),

    /// Whether zstd may keep referencing the input buffer instead of copying
    /// it.
    ///
    /// This requires the input to stay allocated and unmodified until the
    /// frame is complete, which a borrowed `InBuffer` cannot guarantee:
    /// `CCtx::set_parameter` refuses to enable it and returns an error.
    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    StableInBuffer(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    StableOutBuffer(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    BlockDelimiters(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    ValidateSequences(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    UseBlockSplitter(ParamSwitch),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    UseRowMatchFinder(ParamSwitch),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    DeterministicRefPrefix(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    PrefetchCDictTables(ParamSwitch),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    EnableSeqProducerFallback(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    MaxBlockSize(u32),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    SearchForExternalRepcodes(ParamSwitch),

    /// Target CBlock size.
    ///
    /// Tries to make compressed blocks fit in this size (not a guarantee, just a target).
    /// Useful to reduce end-to-end latency in low-bandwidth environments.
    ///
    /// No target when the value is 0.
    TargetCBlockSize(u32),

    /// Compression level to use.
    ///
    /// Compression levels are global presets for the other compression parameters.
    CompressionLevel(CompressionLevel),

    /// Maximum allowed back-reference distance.
    ///
    /// The actual distance is 2 power "this value".
    WindowLog(u32),

    HashLog(u32),

    ChainLog(u32),

    SearchLog(u32),

    MinMatch(u32),

    TargetLength(u32),

    Strategy(Strategy),

    EnableLongDistanceMatching(bool),

    LdmHashLog(u32),

    LdmMinMatch(u32),

    LdmBucketSizeLog(u32),

    LdmHashRateLog(u32),

    ContentSizeFlag(bool),

    ChecksumFlag(bool),

    DictIdFlag(bool),

    /// How many threads will be spawned.
    ///
    /// With a default value of `0`, `compress_stream*` functions block until they complete.
    ///
    /// With any other value (including 1, a single compressing thread), these methods directly
    /// return, and the actual compression is done in the background (until a flush is requested).
    /// A context compressing that way from a borrowed dictionary or prefix
    /// must not be leaked, see [`CCtx`](CCtx#worker-threads-and-leaks).
    ///
    /// Note: this will only work if the `zstdmt` feature is activated.
    NbWorkers(u32),

    /// Size in bytes of a compression job.
    ///
    /// Does not have any effect when `NbWorkers` is set to 0.
    ///
    /// The default value of 0 finds the best job size based on the compression parameters.
    ///
    /// Note: this will only work if the `zstdmt` feature is activated.
    JobSize(u32),

    /// Specifies how much overlap must be given to each worker.
    ///
    /// Possible values:
    ///
    /// * `0` (default value): automatic overlap based on compression strategy.
    /// * `1`: No overlap
    /// * `1 < n < 9`: Overlap a fraction of the window size, defined as `1/(2 ^ 9-n)`.
    /// * `9`: Full overlap (as long as the window)
    /// * `9 < m`: Will return an error.
    ///
    /// Note: this will only work if the `zstdmt` feature is activated.
    OverlapSizeLog(u32),
}

/// A decompression parameter.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum DParameter {
    WindowLogMax(u32),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    /// See `FrameFormat`.
    Format(FrameFormat),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    StableOutBuffer(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    ForceIgnoreChecksum(bool),

    #[cfg(feature = "experimental")]
    #[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
    RefMultipleDDicts(bool),
}

/// Wraps the `ZDICT_trainFromBuffer()` function.
#[cfg(feature = "zdict_builder")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "zdict_builder")))]
pub fn train_from_buffer<C: WriteBuf + ?Sized>(
    dict_buffer: &mut C,
    samples_buffer: &[u8],
    samples_sizes: &[usize],
) -> SafeResult {
    // A plain `sum()` wraps around when overflow checks are disabled, and
    // would accept sizes reaching far outside of `samples_buffer`.
    let total_size = samples_sizes
        .iter()
        .try_fold(0usize, |total, &size| total.checked_add(size));
    assert_eq!(
        Some(samples_buffer.len()),
        total_size,
        "sample sizes don't add up to the samples buffer length"
    );

    unsafe {
        dict_buffer.write_from(|buffer, capacity| {
            parse_code(zstd_sys::ZDICT_trainFromBuffer(
                buffer,
                capacity,
                ptr_void(samples_buffer),
                samples_sizes.as_ptr(),
                samples_sizes.len() as u32,
            ))
        })
    }
}

/// Wraps the `ZDICT_getDictID()` function.
#[cfg(feature = "zdict_builder")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "zdict_builder")))]
pub fn get_dict_id(dict_buffer: &[u8]) -> Option<NonZeroU32> {
    NonZeroU32::new(unsafe {
        zstd_sys::ZDICT_getDictID(ptr_void(dict_buffer), dict_buffer.len())
    })
}

/// Wraps the `ZSTD_getBlockSize()` function.
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn get_block_size(cctx: &CCtx) -> usize {
    unsafe { zstd_sys::ZSTD_getBlockSize(cctx.0.as_ptr()) }
}

/// Wraps the `ZSTD_decompressBound` function
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn decompress_bound(data: &[u8]) -> Result<u64, ErrorCode> {
    let bound =
        unsafe { zstd_sys::ZSTD_decompressBound(ptr_void(data), data.len()) };
    if is_error(bound as usize) {
        Err(bound as usize)
    } else {
        Ok(bound)
    }
}

/// Given a buffer of size `src_size`, returns the maximum number of sequences that can ge
/// generated.
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn sequence_bound(src_size: usize) -> usize {
    // Safety: Just FFI.
    unsafe { zstd_sys::ZSTD_sequenceBound(src_size) }
}

/// Returns the minimum extra space when output and input buffer overlap.
///
/// When using in-place decompression, the output buffer must be at least this much bigger (in
/// bytes) than the input buffer. The extra space must be at the front of the output buffer (the
/// input buffer must be at the end of the output buffer).
#[cfg(feature = "experimental")]
#[cfg_attr(feature = "doc-cfg", doc(cfg(feature = "experimental")))]
pub fn decompression_margin(
    compressed_data: &[u8],
) -> Result<usize, ErrorCode> {
    parse_code(unsafe {
        zstd_sys::ZSTD_decompressionMargin(
            ptr_void(compressed_data),
            compressed_data.len(),
        )
    })
}
