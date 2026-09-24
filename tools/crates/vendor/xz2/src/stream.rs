//! Raw in-memory LZMA streams.
//!
//! The `Stream` type exported by this module is the primary type which performs
//! encoding/decoding of LZMA streams. Each `Stream` is either an encoder or
//! decoder and processes data in a streaming fashion.

use std::collections::LinkedList;
use std::error;
use std::fmt;
use std::io;
use std::mem;
use std::slice;

use lzma_sys;

/// Representation of an in-memory LZMA encoding or decoding stream.
///
/// Wraps the raw underlying `lzma_stream` type and provides the ability to
/// create streams which can either decode or encode various LZMA-based formats.
pub struct Stream {
    raw: lzma_sys::lzma_stream,
}

unsafe impl Send for Stream {}
unsafe impl Sync for Stream {}

/// Options that can be used to configure how LZMA encoding happens.
///
/// This builder is consumed by a number of other methods.
pub struct LzmaOptions {
    raw: lzma_sys::lzma_options_lzma,
}

/// Builder to create a multi-threaded stream encoder.
pub struct MtStreamBuilder {
    raw: lzma_sys::lzma_mt,
    filters: Option<Filters>,
}

/// A custom chain of filters to configure an encoding stream.
pub struct Filters {
    inner: Vec<lzma_sys::lzma_filter>,
    lzma_opts: LinkedList<lzma_sys::lzma_options_lzma>,
}

/// The `action` argument for `process`,
///
/// After the first use of SyncFlush, FullFlush, FullBarrier, or Finish, the
/// same `action' must is used until `process` returns `Status::StreamEnd`.
/// Also, the amount of input must not be modified by the application until
/// `process` returns `Status::StreamEnd`. Changing the `action' or modifying
/// the amount of input will make `process` return `Error::Program`.
#[derive(Copy, Clone)]
pub enum Action {
    /// Continue processing
    ///
    /// When encoding, encode as much input as possible. Some internal buffering
    /// will probably be done (depends on the filter chain in use), which causes
    /// latency: the input used won't usually be decodeable from the output of
    /// the same `process` call.
    ///
    /// When decoding, decode as much input as possible and produce as much
    /// output as possible.
    Run = lzma_sys::LZMA_RUN as isize,

    /// Make all the input available at output
    ///
    /// Normally the encoder introduces some latency. `SyncFlush` forces all the
    /// buffered data to be available at output without resetting the internal
    /// state of the encoder. This way it is possible to use compressed stream
    /// for example for communication over network.
    ///
    /// Only some filters support `SyncFlush`. Trying to use `SyncFlush` with
    /// filters that don't support it will make `process` return
    /// `Error::Options`. For example, LZMA1 doesn't support `SyncFlush` but
    /// LZMA2 does.
    ///
    /// Using `SyncFlush` very often can dramatically reduce the compression
    /// ratio. With some filters (for example, LZMA2), fine-tuning the
    /// compression options may help mitigate this problem significantly (for
    /// example, match finder with LZMA2).
    ///
    /// Decoders don't support `SyncFlush`.
    SyncFlush = lzma_sys::LZMA_SYNC_FLUSH as isize,

    /// Finish encoding of the current block.
    ///
    /// All the input data going to the current block must have been given to
    /// the encoder. Call `process` with `FullFlush` until it returns
    /// `Status::StreamEnd`. Then continue normally with `Run` or finish the
    /// Stream with `Finish`.
    ///
    /// This action is currently supported only by stream encoder and easy
    /// encoder (which uses stream encoder). If there is no unfinished block, no
    /// empty block is created.
    FullFlush = lzma_sys::LZMA_FULL_FLUSH as isize,

    /// Finish encoding of the current block.
    ///
    /// This is like `FullFlush` except that this doesn't necessarily wait until
    /// all the input has been made available via the output buffer. That is,
    /// `process` might return `Status::StreamEnd` as soon as all the input has
    /// been consumed.
    ///
    /// `FullBarrier` is useful with a threaded encoder if one wants to split
    /// the .xz Stream into blocks at specific offsets but doesn't care if the
    /// output isn't flushed immediately. Using `FullBarrier` allows keeping the
    /// threads busy while `FullFlush` would make `process` wait until all the
    /// threads have finished until more data could be passed to the encoder.
    ///
    /// With a `Stream` initialized with the single-threaded
    /// `new_stream_encoder` or `new_easy_encoder`, `FullBarrier` is an alias
    /// for `FullFlush`.
    FullBarrier = lzma_sys::LZMA_FULL_BARRIER as isize,

    /// Finish the current operation
    ///
    /// All the input data must have been given to the encoder (the last bytes
    /// can still be pending in next_in). Call `process` with `Finish` until it
    /// returns `Status::StreamEnd`. Once `Finish` has been used, the amount of
    /// input must no longer be changed by the application.
    ///
    /// When decoding, using `Finish` is optional unless the concatenated flag
    /// was used when the decoder was initialized. When concatenated was not
    /// used, the only effect of `Finish` is that the amount of input must not
    /// be changed just like in the encoder.
    Finish = lzma_sys::LZMA_FINISH as isize,
}

/// Return value of a `process` operation.
#[derive(Debug, Copy, Clone, PartialEq)]
pub enum Status {
    /// Operation completed successfully.
    Ok,

    /// End of stream was reached.
    ///
    /// When encoding, this means that a sync/full flush or `Finish` was
    /// completed. When decoding, this indicates that all data was decoded
    /// successfully.
    StreamEnd,

    /// If the TELL_ANY_CHECK flags is specified when constructing a decoder,
    /// this informs that the `check` method will now return the underlying
    /// integrity check algorithm.
    GetCheck,

    /// An error has not been encountered, but no progress is possible.
    ///
    /// Processing can be continued normally by providing more input and/or more
    /// output space, if possible.
    ///
    /// Typically the first call to `process` that can do no progress returns
    /// `Ok` instead of `MemNeeded`. Only the second consecutive call doing no
    /// progress will return `MemNeeded`.
    MemNeeded,
}

/// Possible error codes that can be returned from a processing operation.
#[derive(Debug, Clone, PartialEq)]
pub enum Error {
    /// The underlying data was corrupt.
    Data,

    /// Invalid or unsupported options were specified.
    Options,

    /// File format wasn't recognized.
    Format,

    /// Memory usage limit was reached.
    ///
    /// The memory limit can be increased with `set_memlimit`
    MemLimit,

    /// Memory couldn't be allocated.
    Mem,

    /// A programming error was encountered.
    Program,

    /// The `TELL_NO_CHECK` flag was specified and no integrity check was
    /// available for this stream.
    NoCheck,

    /// The `TELL_UNSUPPORTED_CHECK` flag was specified and no integrity check
    /// isn't implemented in this build of liblzma for this stream.
    UnsupportedCheck,
}

/// Possible integrity checks that can be part of a .xz stream.
#[allow(missing_docs)] // self explanatory mostly
#[derive(Copy, Clone)]
pub enum Check {
    None = lzma_sys::LZMA_CHECK_NONE as isize,
    Crc32 = lzma_sys::LZMA_CHECK_CRC32 as isize,
    Crc64 = lzma_sys::LZMA_CHECK_CRC64 as isize,
    Sha256 = lzma_sys::LZMA_CHECK_SHA256 as isize,
}

/// Compression modes
///
/// This selects the function used to analyze the data produced by the match
/// finder.
#[derive(Copy, Clone)]
pub enum Mode {
    /// Fast compression.
    ///
    /// Fast mode is usually at its best when combined with a hash chain match
    /// finder.
    Fast = lzma_sys::LZMA_MODE_FAST as isize,

    /// Normal compression.
    ///
    /// This is usually notably slower than fast mode. Use this together with
    /// binary tree match finders to expose the full potential of the LZMA1 or
    /// LZMA2 encoder.
    Normal = lzma_sys::LZMA_MODE_NORMAL as isize,
}

/// Match finders
///
/// Match finder has major effect on both speed and compression ratio. Usually
/// hash chains are faster than binary trees.
///
/// If you will use `SyncFlush` often, the hash chains may be a better choice,
/// because binary trees get much higher compression ratio penalty with
/// `SyncFlush`.
///
/// The memory usage formulas are only rough estimates, which are closest to
/// reality when dict_size is a power of two. The formulas are  more complex in
/// reality, and can also change a little between liblzma versions.
#[derive(Copy, Clone)]
pub enum MatchFinder {
    /// Hash Chain with 2- and 3-byte hashing
    HashChain3 = lzma_sys::LZMA_MF_HC3 as isize,
    /// Hash Chain with 2-, 3-, and 4-byte hashing
    HashChain4 = lzma_sys::LZMA_MF_HC4 as isize,

    /// Binary Tree with 2-byte hashing
    BinaryTree2 = lzma_sys::LZMA_MF_BT2 as isize,
    /// Binary Tree with 2- and 3-byte hashing
    BinaryTree3 = lzma_sys::LZMA_MF_BT3 as isize,
    /// Binary Tree with 2-, 3-, and 4-byte hashing
    BinaryTree4 = lzma_sys::LZMA_MF_BT4 as isize,
}

/// A flag passed when initializing a decoder, causes `process` to return
/// `Status::GetCheck` as soon as the integrity check is known.
pub const TELL_ANY_CHECK: u32 = lzma_sys::LZMA_TELL_ANY_CHECK;

/// A flag passed when initializing a decoder, causes `process` to return
/// `Error::NoCheck` if the stream being decoded has no integrity check.
pub const TELL_NO_CHECK: u32 = lzma_sys::LZMA_TELL_NO_CHECK;

/// A flag passed when initializing a decoder, causes `process` to return
/// `Error::UnsupportedCheck` if the stream being decoded has an integrity check
/// that cannot be verified by this build of liblzma.
pub const TELL_UNSUPPORTED_CHECK: u32 = lzma_sys::LZMA_TELL_UNSUPPORTED_CHECK;

/// A flag passed when initializing a decoder, causes the decoder to ignore any
/// integrity checks listed.
pub const IGNORE_CHECK: u32 = lzma_sys::LZMA_TELL_UNSUPPORTED_CHECK;

/// A flag passed when initializing a decoder, indicates that the stream may be
/// multiple concatenated xz files.
pub const CONCATENATED: u32 = lzma_sys::LZMA_CONCATENATED;

impl Stream {
    /// Initialize .xz stream encoder using a preset number
    ///
    /// This is intended to be used by most for encoding data. The `preset`
    /// argument is a number 0-9 indicating the compression level to use, and
    /// normally 6 is a reasonable default.
    ///
    /// The `check` argument is the integrity check to insert at the end of the
    /// stream. The default of `Crc64` is typically appropriate.
    pub fn new_easy_encoder(preset: u32, check: Check) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_easy_encoder(
                &mut init.raw,
                preset,
                check as lzma_sys::lzma_check,
            ))?;
            Ok(init)
        }
    }

    /// Initialize .lzma encoder (legacy file format)
    ///
    /// The .lzma format is sometimes called the LZMA_Alone format, which is the
    /// reason for the name of this function. The .lzma format supports only the
    /// LZMA1 filter. There is no support for integrity checks like CRC32.
    ///
    /// Use this function if and only if you need to create files readable by
    /// legacy LZMA tools such as LZMA Utils 4.32.x. Moving to the .xz format
    /// (the `new_easy_encoder` function) is strongly recommended.
    ///
    /// The valid action values for `process` are `Run` and `Finish`. No kind
    /// of flushing is supported, because the file format doesn't make it
    /// possible.
    pub fn new_lzma_encoder(options: &LzmaOptions) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_alone_encoder(&mut init.raw, &options.raw))?;
            Ok(init)
        }
    }

    /// Initialize .xz Stream encoder using a custom filter chain
    ///
    /// This function is similar to `new_easy_encoder` but a custom filter chain
    /// is specified.
    pub fn new_stream_encoder(filters: &Filters, check: Check) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_stream_encoder(
                &mut init.raw,
                filters.inner.as_ptr(),
                check as lzma_sys::lzma_check,
            ))?;
            Ok(init)
        }
    }

    /// Initialize a .xz stream decoder.
    ///
    /// The maximum memory usage can be specified along with flags such as
    /// `TELL_ANY_CHECK`, `TELL_NO_CHECK`, `TELL_UNSUPPORTED_CHECK`,
    /// `TELL_IGNORE_CHECK`, or `CONCATENATED`.
    pub fn new_stream_decoder(memlimit: u64, flags: u32) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_stream_decoder(
                &mut init.raw,
                memlimit,
                flags,
            ))?;
            Ok(init)
        }
    }

    /// Initialize a .lzma stream decoder.
    ///
    /// The maximum memory usage can also be specified.
    pub fn new_lzma_decoder(memlimit: u64) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_alone_decoder(&mut init.raw, memlimit))?;
            Ok(init)
        }
    }

    /// Initialize a decoder which will choose a stream/lzma formats depending
    /// on the input stream.
    pub fn new_auto_decoder(memlimit: u64, flags: u32) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_auto_decoder(&mut init.raw, memlimit, flags))?;
            Ok(init)
        }
    }

    /// Processes some data from input into an output buffer.
    ///
    /// This will perform the appropriate encoding or decoding operation
    /// depending on the kind of underlying stream. Documentation for the
    /// various `action` arguments can be found on the respective variants.
    pub fn process(
        &mut self,
        input: &[u8],
        output: &mut [u8],
        action: Action,
    ) -> Result<Status, Error> {
        self.raw.next_in = input.as_ptr();
        self.raw.avail_in = input.len();
        self.raw.next_out = output.as_mut_ptr();
        self.raw.avail_out = output.len();
        let action = action as lzma_sys::lzma_action;
        unsafe { cvt(lzma_sys::lzma_code(&mut self.raw, action)) }
    }

    /// Performs the same data as `process`, but places output data in a `Vec`.
    ///
    /// This function will use the extra capacity of `output` as a destination
    /// for bytes to be placed. The length of `output` will automatically get
    /// updated after the operation has completed.
    pub fn process_vec(
        &mut self,
        input: &[u8],
        output: &mut Vec<u8>,
        action: Action,
    ) -> Result<Status, Error> {
        let cap = output.capacity();
        let len = output.len();

        unsafe {
            let before = self.total_out();
            let ret = {
                let ptr = output.as_mut_ptr().offset(len as isize);
                let out = slice::from_raw_parts_mut(ptr, cap - len);
                self.process(input, out, action)
            };
            output.set_len((self.total_out() - before) as usize + len);
            return ret;
        }
    }

    /// Returns the total amount of input bytes consumed by this stream.
    pub fn total_in(&self) -> u64 {
        self.raw.total_in
    }

    /// Returns the total amount of bytes produced by this stream.
    pub fn total_out(&self) -> u64 {
        self.raw.total_out
    }

    /// Get the current memory usage limit.
    ///
    /// This is only supported if the underlying stream supports a memlimit.
    pub fn memlimit(&self) -> u64 {
        unsafe { lzma_sys::lzma_memlimit_get(&self.raw) }
    }

    /// Set the current memory usage limit.
    ///
    /// This can return `Error::MemLimit` if the new limit is too small or
    /// `Error::Program` if this stream doesn't take a memory limit.
    pub fn set_memlimit(&mut self, limit: u64) -> Result<(), Error> {
        cvt(unsafe { lzma_sys::lzma_memlimit_set(&mut self.raw, limit) }).map(|_| ())
    }
}

impl LzmaOptions {
    /// Creates a new blank set of options for encoding.
    ///
    /// The `preset` argument is the compression level to use, typically in the
    /// range of 0-9.
    pub fn new_preset(preset: u32) -> Result<LzmaOptions, Error> {
        unsafe {
            let mut options = LzmaOptions { raw: mem::zeroed() };
            let ret = lzma_sys::lzma_lzma_preset(&mut options.raw, preset);
            if ret != 0 {
                Err(Error::Program)
            } else {
                Ok(options)
            }
        }
    }

    /// Configures the dictionary size, in bytes
    ///
    /// Dictionary size indicates how many bytes of the recently processed
    /// uncompressed data is kept in memory.
    ///
    /// The minimum dictionary size is 4096 bytes and the default is 2^23, 8MB.
    pub fn dict_size(&mut self, size: u32) -> &mut LzmaOptions {
        self.raw.dict_size = size;
        self
    }

    /// Configures the number of literal context bits.
    ///
    /// How many of the highest bits of the previous uncompressed eight-bit byte
    /// (also known as `literal') are taken into account when predicting the
    /// bits of the next literal.
    ///
    /// The maximum value to this is 4 and the default is 3. It is not currently
    /// supported if this plus `literal_position_bits` is greater than 4.
    pub fn literal_context_bits(&mut self, bits: u32) -> &mut LzmaOptions {
        self.raw.lc = bits;
        self
    }

    /// Configures the number of literal position bits.
    ///
    /// This affects what kind of alignment in the uncompressed data is assumed
    /// when encoding literals. A literal is a single 8-bit byte. See
    /// `position_bits` for more information about alignment.
    ///
    /// The default for this is 0.
    pub fn literal_position_bits(&mut self, bits: u32) -> &mut LzmaOptions {
        self.raw.lp = bits;
        self
    }

    /// Configures the number of position bits.
    ///
    /// Position bits affects what kind of alignment in the uncompressed data is
    /// assumed in general. The default of 2 means four-byte alignment (2^ pb
    /// =2^2=4), which is often a good choice when there's no better guess.
    ///
    /// When the aligment is known, setting pb accordingly may reduce the file
    /// size a little. E.g. with text files having one-byte alignment (US-ASCII,
    /// ISO-8859-*, UTF-8), setting pb=0 can improve compression slightly. For
    /// UTF-16 text, pb=1 is a good choice. If the alignment is an odd number
    /// like 3 bytes, pb=0 might be the best choice.
    ///
    /// Even though the assumed alignment can be adjusted with pb and lp, LZMA1
    /// and LZMA2 still slightly favor 16-byte alignment. It might be worth
    /// taking into account when designing file formats that are likely to be
    /// often compressed with LZMA1 or LZMA2.
    pub fn position_bits(&mut self, bits: u32) -> &mut LzmaOptions {
        self.raw.pb = bits;
        self
    }

    /// Configures the compression mode.
    pub fn mode(&mut self, mode: Mode) -> &mut LzmaOptions {
        self.raw.mode = mode as lzma_sys::lzma_mode;
        self
    }

    /// Configures the nice length of a match.
    ///
    /// This determines how many bytes the encoder compares from the match
    /// candidates when looking for the best match. Once a match of at least
    /// `nice_len` bytes long is found, the encoder stops looking for better
    /// candidates and encodes the match. (Naturally, if the found match is
    /// actually longer than `nice_len`, the actual length is encoded; it's not
    /// truncated to `nice_len`.)
    ///
    /// Bigger values usually increase the compression ratio and compression
    /// time. For most files, 32 to 128 is a good value, which gives very good
    /// compression ratio at good speed.
    ///
    /// The exact minimum value depends on the match finder. The maximum is 273,
    /// which is the maximum length of a match that LZMA1 and LZMA2 can encode.
    pub fn nice_len(&mut self, len: u32) -> &mut LzmaOptions {
        self.raw.nice_len = len;
        self
    }

    /// Configures the match finder ID.
    pub fn match_finder(&mut self, mf: MatchFinder) -> &mut LzmaOptions {
        self.raw.mf = mf as lzma_sys::lzma_match_finder;
        self
    }

    /// Maximum search depth in the match finder.
    ///
    /// For every input byte, match finder searches through the hash chain or
    /// binary tree in a loop, each iteration going one step deeper in the chain
    /// or tree. The searching stops if
    ///
    ///  - a match of at least `nice_len` bytes long is found;
    ///  - all match candidates from the hash chain or binary tree have
    ///    been checked; or
    ///  - maximum search depth is reached.
    ///
    /// Maximum search depth is needed to prevent the match finder from wasting
    /// too much time in case there are lots of short match candidates. On the
    /// other hand, stopping the search before all candidates have been checked
    /// can reduce compression ratio.
    ///
    /// Setting depth to zero tells liblzma to use an automatic default value,
    /// that depends on the selected match finder and nice_len.  The default is
    /// in the range [4, 200] or so (it may vary between liblzma versions).
    ///
    /// Using a bigger depth value than the default can increase compression
    /// ratio in some cases. There is no strict maximum value, but high values
    /// (thousands or millions) should be used with care: the encoder could
    /// remain fast enough with typical input, but malicious input could cause
    /// the match finder to slow down dramatically, possibly creating a denial
    /// of service attack.
    pub fn depth(&mut self, depth: u32) -> &mut LzmaOptions {
        self.raw.depth = depth;
        self
    }
}

impl Check {
    /// Test if this check is supported in this build of liblzma.
    pub fn is_supported(&self) -> bool {
        let ret = unsafe { lzma_sys::lzma_check_is_supported(*self as lzma_sys::lzma_check) };
        ret != 0
    }
}

impl MatchFinder {
    /// Test if this match finder is supported in this build of liblzma.
    pub fn is_supported(&self) -> bool {
        let ret = unsafe { lzma_sys::lzma_mf_is_supported(*self as lzma_sys::lzma_match_finder) };
        ret != 0
    }
}

impl Filters {
    /// Creates a new filter chain with no filters.
    pub fn new() -> Filters {
        Filters {
            inner: vec![lzma_sys::lzma_filter {
                id: lzma_sys::LZMA_VLI_UNKNOWN,
                options: 0 as *mut _,
            }],
            lzma_opts: LinkedList::new(),
        }
    }

    /// Add an LZMA1 filter.
    ///
    /// LZMA1 is the very same thing as what was called just LZMA in LZMA Utils,
    /// 7-Zip, and LZMA SDK. It's called LZMA1 here to prevent developers from
    /// accidentally using LZMA when they actually want LZMA2.
    ///
    /// LZMA1 shouldn't be used for new applications unless you _really_ know
    /// what you are doing.  LZMA2 is almost always a better choice.
    pub fn lzma1(&mut self, opts: &LzmaOptions) -> &mut Filters {
        self.lzma_opts.push_back(opts.raw);
        let ptr = self.lzma_opts.back().unwrap() as *const _ as *mut _;
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_LZMA1,
            options: ptr,
        })
    }

    /// Add an LZMA2 filter.
    ///
    /// Usually you want this instead of LZMA1. Compared to LZMA1, LZMA2 adds
    /// support for `SyncFlush`, uncompressed chunks (smaller expansion when
    /// trying to compress uncompressible data), possibility to change
    /// `literal_context_bits`/`literal_position_bits`/`position_bits` in the
    /// middle of encoding, and some other internal improvements.
    pub fn lzma2(&mut self, opts: &LzmaOptions) -> &mut Filters {
        self.lzma_opts.push_back(opts.raw);
        let ptr = self.lzma_opts.back().unwrap() as *const _ as *mut _;
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_LZMA2,
            options: ptr,
        })
    }

    // TODO: delta filter

    /// Add a filter for x86 binaries.
    pub fn x86(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_X86,
            options: 0 as *mut _,
        })
    }

    /// Add a filter for PowerPC binaries.
    pub fn powerpc(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_POWERPC,
            options: 0 as *mut _,
        })
    }

    /// Add a filter for IA-64 (itanium) binaries.
    pub fn ia64(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_IA64,
            options: 0 as *mut _,
        })
    }

    /// Add a filter for ARM binaries.
    pub fn arm(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_ARM,
            options: 0 as *mut _,
        })
    }

    /// Add a filter for ARM-Thumb binaries.
    pub fn arm_thumb(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_ARMTHUMB,
            options: 0 as *mut _,
        })
    }

    /// Add a filter for SPARC binaries.
    pub fn sparc(&mut self) -> &mut Filters {
        self.push(lzma_sys::lzma_filter {
            id: lzma_sys::LZMA_FILTER_SPARC,
            options: 0 as *mut _,
        })
    }

    fn push(&mut self, filter: lzma_sys::lzma_filter) -> &mut Filters {
        let pos = self.inner.len() - 1;
        self.inner.insert(pos, filter);
        self
    }
}

impl MtStreamBuilder {
    /// Creates a new blank builder to create a multithreaded encoding `Stream`.
    pub fn new() -> MtStreamBuilder {
        unsafe {
            let mut init = MtStreamBuilder {
                raw: mem::zeroed(),
                filters: None,
            };
            init.raw.threads = 1;
            return init;
        }
    }

    /// Configures the number of worker threads to use
    pub fn threads(&mut self, threads: u32) -> &mut Self {
        self.raw.threads = threads;
        self
    }

    /// Configures the maximum uncompressed size of a block
    ///
    /// The encoder will start a new .xz block every `block_size` bytes.
    /// Using `FullFlush` or `FullBarrier` with `process` the caller may tell
    /// liblzma to start a new block earlier.
    ///
    /// With LZMA2, a recommended block size is 2-4 times the LZMA2 dictionary
    /// size. With very small dictionaries, it is recommended to use at least 1
    /// MiB block size for good compression ratio, even if this is more than
    /// four times the dictionary size. Note that these are only recommendations
    /// for typical use cases; feel free to use other values. Just keep in mind
    /// that using a block size less than the LZMA2 dictionary size is waste of
    /// RAM.
    ///
    /// Set this to 0 to let liblzma choose the block size depending on the
    /// compression options. For LZMA2 it will be 3*`dict_size` or 1 MiB,
    /// whichever is more.
    ///
    /// For each thread, about 3 * `block_size` bytes of memory will be
    /// allocated. This may change in later liblzma versions. If so, the memory
    /// usage will probably be reduced, not increased.
    pub fn block_size(&mut self, block_size: u64) -> &mut Self {
        self.raw.block_size = block_size;
        self
    }

    /// Timeout to allow `process` to return early
    ///
    /// Multithreading can make liblzma to consume input and produce output in a
    /// very bursty way: it may first read a lot of input to fill internal
    /// buffers, then no input or output occurs for a while.
    ///
    /// In single-threaded mode, `process` won't return until it has either
    /// consumed all the input or filled the output buffer. If this is done in
    /// multithreaded mode, it may cause a call `process` to take even tens of
    /// seconds, which isn't acceptable in all applications.
    ///
    /// To avoid very long blocking times in `process`, a timeout (in
    /// milliseconds) may be set here. If `process would block longer than
    /// this number of milliseconds, it will return with `Ok`. Reasonable
    /// values are 100 ms or more. The xz command line tool uses 300 ms.
    ///
    /// If long blocking times are fine for you, set timeout to a special
    /// value of 0, which will disable the timeout mechanism and will make
    /// `process` block until all the input is consumed or the output
    /// buffer has been filled.
    pub fn timeout_ms(&mut self, timeout: u32) -> &mut Self {
        self.raw.timeout = timeout;
        self
    }

    /// Compression preset (level and possible flags)
    ///
    /// The preset is set just like with `Stream::new_easy_encoder`. The preset
    /// is ignored if filters below have been specified.
    pub fn preset(&mut self, preset: u32) -> &mut Self {
        self.raw.preset = preset;
        self
    }

    /// Configure a custom filter chain
    pub fn filters(&mut self, filters: Filters) -> &mut Self {
        self.raw.filters = filters.inner.as_ptr();
        self.filters = Some(filters);
        self
    }

    /// Configures the integrity check type
    pub fn check(&mut self, check: Check) -> &mut Self {
        self.raw.check = check as lzma_sys::lzma_check;
        self
    }

    /// Calculate approximate memory usage of multithreaded .xz encoder
    pub fn memusage(&self) -> u64 {
        unsafe { lzma_sys::lzma_stream_encoder_mt_memusage(&self.raw) }
    }

    /// Initialize multithreaded .xz stream encoder.
    pub fn encoder(&self) -> Result<Stream, Error> {
        unsafe {
            let mut init = Stream { raw: mem::zeroed() };
            cvt(lzma_sys::lzma_stream_encoder_mt(&mut init.raw, &self.raw))?;
            Ok(init)
        }
    }
}

fn cvt(rc: lzma_sys::lzma_ret) -> Result<Status, Error> {
    match rc {
        lzma_sys::LZMA_OK => Ok(Status::Ok),
        lzma_sys::LZMA_STREAM_END => Ok(Status::StreamEnd),
        lzma_sys::LZMA_NO_CHECK => Err(Error::NoCheck),
        lzma_sys::LZMA_UNSUPPORTED_CHECK => Err(Error::UnsupportedCheck),
        lzma_sys::LZMA_GET_CHECK => Ok(Status::GetCheck),
        lzma_sys::LZMA_MEM_ERROR => Err(Error::Mem),
        lzma_sys::LZMA_MEMLIMIT_ERROR => Err(Error::MemLimit),
        lzma_sys::LZMA_FORMAT_ERROR => Err(Error::Format),
        lzma_sys::LZMA_OPTIONS_ERROR => Err(Error::Options),
        lzma_sys::LZMA_DATA_ERROR => Err(Error::Data),
        lzma_sys::LZMA_BUF_ERROR => Ok(Status::MemNeeded),
        lzma_sys::LZMA_PROG_ERROR => Err(Error::Program),
        c => panic!("unknown return code: {}", c),
    }
}

impl From<Error> for io::Error {
    fn from(e: Error) -> io::Error {
        let kind = match e {
            Error::Data => std::io::ErrorKind::InvalidData,
            Error::Options => std::io::ErrorKind::InvalidInput,
            Error::Format => std::io::ErrorKind::InvalidData,
            Error::MemLimit => std::io::ErrorKind::Other,
            Error::Mem => std::io::ErrorKind::Other,
            Error::Program => std::io::ErrorKind::Other,
            Error::NoCheck => std::io::ErrorKind::InvalidInput,
            Error::UnsupportedCheck => std::io::ErrorKind::Other,
        };

        io::Error::new(kind, e)
    }
}

impl error::Error for Error {}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::Data => "lzma data error",
            Error::Options => "invalid options",
            Error::Format => "stream/file format not recognized",
            Error::MemLimit => "memory limit reached",
            Error::Mem => "can't allocate memory",
            Error::Program => "liblzma internal error",
            Error::NoCheck => "no integrity check was available",
            Error::UnsupportedCheck => "liblzma not built with check support",
        }
        .fmt(f)
    }
}

impl Drop for Stream {
    fn drop(&mut self) {
        unsafe {
            lzma_sys::lzma_end(&mut self.raw);
        }
    }
}
