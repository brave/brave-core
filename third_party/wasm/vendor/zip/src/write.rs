//! Types for creating ZIP archives

use crate::compression::CompressionMethod;
use crate::read::{find_content, ZipArchive, ZipFile, ZipFileReader};
use crate::result::{ZipError, ZipResult};
use crate::spec;
use crate::types::{ffi, DateTime, System, ZipFileData, DEFAULT_VERSION};
#[cfg(any(feature = "_deflate-any", feature = "bzip2", feature = "zstd",))]
use core::num::NonZeroU64;
use crc32fast::Hasher;
use indexmap::IndexMap;
use std::default::Default;
use std::io;
use std::io::prelude::*;
use std::io::{BufReader, SeekFrom};
use std::mem;
use std::str::{from_utf8, Utf8Error};
use std::sync::{Arc, OnceLock};

#[cfg(any(
    feature = "deflate",
    feature = "deflate-zlib",
    feature = "deflate-zlib-ng"
))]
use flate2::{write::DeflateEncoder, Compression};

#[cfg(feature = "bzip2")]
use bzip2::write::BzEncoder;

#[cfg(feature = "time")]
use time::OffsetDateTime;

#[cfg(feature = "deflate-zopfli")]
use zopfli::Options;

#[cfg(feature = "deflate-zopfli")]
use std::io::BufWriter;
use std::path::Path;

#[cfg(feature = "zstd")]
use zstd::stream::write::Encoder as ZstdEncoder;

enum MaybeEncrypted<W> {
    Unencrypted(W),
    Encrypted(crate::zipcrypto::ZipCryptoWriter<W>),
}
impl<W: Write> Write for MaybeEncrypted<W> {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        match self {
            MaybeEncrypted::Unencrypted(w) => w.write(buf),
            MaybeEncrypted::Encrypted(w) => w.write(buf),
        }
    }
    fn flush(&mut self) -> io::Result<()> {
        match self {
            MaybeEncrypted::Unencrypted(w) => w.flush(),
            MaybeEncrypted::Encrypted(w) => w.flush(),
        }
    }
}
enum GenericZipWriter<W: Write + Seek> {
    Closed,
    Storer(MaybeEncrypted<W>),
    #[cfg(any(
        feature = "deflate",
        feature = "deflate-zlib",
        feature = "deflate-zlib-ng"
    ))]
    Deflater(DeflateEncoder<MaybeEncrypted<W>>),
    #[cfg(feature = "deflate-zopfli")]
    ZopfliDeflater(zopfli::DeflateEncoder<MaybeEncrypted<W>>),
    #[cfg(feature = "deflate-zopfli")]
    BufferedZopfliDeflater(BufWriter<zopfli::DeflateEncoder<MaybeEncrypted<W>>>),
    #[cfg(feature = "bzip2")]
    Bzip2(BzEncoder<MaybeEncrypted<W>>),
    #[cfg(feature = "zstd")]
    Zstd(ZstdEncoder<'static, MaybeEncrypted<W>>),
}

// Put the struct declaration in a private module to convince rustdoc to display ZipWriter nicely
pub(crate) mod zip_writer {
    use super::*;
    /// ZIP archive generator
    ///
    /// Handles the bookkeeping involved in building an archive, and provides an
    /// API to edit its contents.
    ///
    /// ```
    /// # fn doit() -> zip::result::ZipResult<()>
    /// # {
    /// # use zip::ZipWriter;
    /// use std::io::Write;
    /// use zip::write::SimpleFileOptions;
    ///
    /// // We use a buffer here, though you'd normally use a `File`
    /// let mut buf = [0; 65536];
    /// let mut zip = ZipWriter::new(std::io::Cursor::new(&mut buf[..]));
    ///
    /// let options = SimpleFileOptions::default().compression_method(zip::CompressionMethod::Stored);
    /// zip.start_file("hello_world.txt", options)?;
    /// zip.write(b"Hello, World!")?;
    ///
    /// // Apply the changes you've made.
    /// // Dropping the `ZipWriter` will have the same effect, but may silently fail
    /// zip.finish()?;
    ///
    /// # Ok(())
    /// # }
    /// # doit().unwrap();
    /// ```
    pub struct ZipWriter<W: Write + Seek> {
        pub(super) inner: GenericZipWriter<W>,
        pub(super) files: IndexMap<Box<str>, ZipFileData>,
        pub(super) stats: ZipWriterStats,
        pub(super) writing_to_file: bool,
        pub(super) writing_raw: bool,
        pub(super) comment: Vec<u8>,
        pub(super) flush_on_finish_file: bool,
    }
}
#[doc(inline)]
pub use self::sealed::FileOptionExtension;
use crate::result::ZipError::InvalidArchive;
#[cfg(feature = "lzma")]
use crate::result::ZipError::UnsupportedArchive;
use crate::spec::path_to_string;
use crate::unstable::LittleEndianWriteExt;
use crate::write::GenericZipWriter::{Closed, Storer};
use crate::zipcrypto::ZipCryptoKeys;
use crate::CompressionMethod::Stored;
pub use zip_writer::ZipWriter;

#[derive(Default)]
struct ZipWriterStats {
    hasher: Hasher,
    start: u64,
    bytes_written: u64,
}

struct ZipRawValues {
    crc32: u32,
    compressed_size: u64,
    uncompressed_size: u64,
}
mod sealed {
    use std::sync::Arc;

    use super::ExtendedFileOptions;

    pub trait Sealed {}
    /// File options Extensions
    #[doc(hidden)]
    pub trait FileOptionExtension: Default + Sealed {
        /// Extra Data
        fn extra_data(&self) -> Option<&Arc<Vec<u8>>>;
        /// Central Extra Data
        fn central_extra_data(&self) -> Option<&Arc<Vec<u8>>>;
    }
    impl Sealed for () {}
    impl FileOptionExtension for () {
        fn extra_data(&self) -> Option<&Arc<Vec<u8>>> {
            None
        }
        fn central_extra_data(&self) -> Option<&Arc<Vec<u8>>> {
            None
        }
    }
    impl Sealed for ExtendedFileOptions {}

    impl FileOptionExtension for ExtendedFileOptions {
        fn extra_data(&self) -> Option<&Arc<Vec<u8>>> {
            Some(&self.extra_data)
        }
        fn central_extra_data(&self) -> Option<&Arc<Vec<u8>>> {
            Some(&self.central_extra_data)
        }
    }
}

/// Metadata for a file to be written
#[derive(Clone, Debug, Copy)]
pub struct FileOptions<T: FileOptionExtension> {
    pub(crate) compression_method: CompressionMethod,
    pub(crate) compression_level: Option<i64>,
    pub(crate) last_modified_time: DateTime,
    pub(crate) permissions: Option<u32>,
    pub(crate) large_file: bool,
    encrypt_with: Option<ZipCryptoKeys>,
    extended_options: T,
    alignment: u16,
    #[cfg(feature = "deflate-zopfli")]
    pub(super) zopfli_buffer_size: Option<usize>,
}
/// Simple File Options. Can be copied and good for simple writing zip files
pub type SimpleFileOptions = FileOptions<()>;
/// Adds Extra Data and Central Extra Data. It does not implement copy.
pub type FullFileOptions = FileOptions<ExtendedFileOptions>;
/// The Extension for Extra Data and Central Extra Data
#[derive(Clone, Debug, Default)]
pub struct ExtendedFileOptions {
    extra_data: Arc<Vec<u8>>,
    central_extra_data: Arc<Vec<u8>>,
}

#[cfg(fuzzing)]
impl arbitrary::Arbitrary<'_> for FileOptions<ExtendedFileOptions> {
    fn arbitrary(u: &mut arbitrary::Unstructured) -> arbitrary::Result<Self> {
        let mut options = FullFileOptions {
            compression_method: CompressionMethod::arbitrary(u)?,
            compression_level: None,
            last_modified_time: DateTime::arbitrary(u)?,
            permissions: Option::<u32>::arbitrary(u)?,
            large_file: bool::arbitrary(u)?,
            encrypt_with: Option::<ZipCryptoKeys>::arbitrary(u)?,
            alignment: u16::arbitrary(u)?,
            #[cfg(feature = "deflate-zopfli")]
            zopfli_buffer_size: None,
            ..Default::default()
        };
        match options.compression_method {
            #[cfg(feature = "deflate-zopfli")]
            CompressionMethod::Deflated => {
                if bool::arbitrary(u)? {
                    let level = u.int_in_range(0..=24)?;
                    options.compression_level = Some(level);
                    if level > Compression::best().level().try_into().unwrap() {
                        options.zopfli_buffer_size = Some(1 << u.int_in_range(9..=30)?);
                    }
                }
            }
            Stored => {
                if bool::arbitrary(u)? {
                    options.compression_level = Some(1);
                }
            }
            _ => {
                if bool::arbitrary(u)? {
                    options.compression_level = Some(u.int_in_range(0..=10)?);
                }
            }
        }
        u.arbitrary_loop(Some(0), Some((u16::MAX / 4) as u32), |u| {
            options
                .add_extra_data(
                    u16::arbitrary(u)?,
                    &Vec::<u8>::arbitrary(u)?,
                    bool::arbitrary(u)?,
                )
                .map_err(|_| arbitrary::Error::IncorrectFormat)?;
            Ok(core::ops::ControlFlow::Continue(()))
        })?;
        Ok(options)
    }
}

impl<T: FileOptionExtension> FileOptions<T> {
    /// Set the compression method for the new file
    ///
    /// The default is `CompressionMethod::Deflated` if it is enabled. If not,
    /// `CompressionMethod::Bzip2` is the default if it is enabled. If neither `bzip2` nor `deflate`
    /// is enabled, `CompressionMethod::Zlib` is the default. If all else fails,
    /// `CompressionMethod::Stored` becomes the default and files are written uncompressed.
    #[must_use]
    pub const fn compression_method(mut self, method: CompressionMethod) -> Self {
        self.compression_method = method;
        self
    }

    /// Set the compression level for the new file
    ///
    /// `None` value specifies default compression level.
    ///
    /// Range of values depends on compression method:
    /// * `Deflated`: 10 - 264 for Zopfli, 0 - 9 for other encoders. Default is 24 if Zopfli is the
    ///   only encoder, or 6 otherwise.
    /// * `Bzip2`: 0 - 9. Default is 6
    /// * `Zstd`: -7 - 22, with zero being mapped to default level. Default is 3
    /// * others: only `None` is allowed
    #[must_use]
    pub const fn compression_level(mut self, level: Option<i64>) -> Self {
        self.compression_level = level;
        self
    }

    /// Set the last modified time
    ///
    /// The default is the current timestamp if the 'time' feature is enabled, and 1980-01-01
    /// otherwise
    #[must_use]
    pub const fn last_modified_time(mut self, mod_time: DateTime) -> Self {
        self.last_modified_time = mod_time;
        self
    }

    /// Set the permissions for the new file.
    ///
    /// The format is represented with unix-style permissions.
    /// The default is `0o644`, which represents `rw-r--r--` for files,
    /// and `0o755`, which represents `rwxr-xr-x` for directories.
    ///
    /// This method only preserves the file permissions bits (via a `& 0o777`) and discards
    /// higher file mode bits. So it cannot be used to denote an entry as a directory,
    /// symlink, or other special file type.
    #[must_use]
    pub const fn unix_permissions(mut self, mode: u32) -> Self {
        self.permissions = Some(mode & 0o777);
        self
    }

    /// Set whether the new file's compressed and uncompressed size is less than 4 GiB.
    ///
    /// If set to `false` and the file exceeds the limit, an I/O error is thrown and the file is
    /// aborted. If set to `true`, readers will require ZIP64 support and if the file does not
    /// exceed the limit, 20 B are wasted. The default is `false`.
    #[must_use]
    pub const fn large_file(mut self, large: bool) -> Self {
        self.large_file = large;
        self
    }
    pub(crate) fn with_deprecated_encryption(mut self, password: &[u8]) -> Self {
        self.encrypt_with = Some(ZipCryptoKeys::derive(password));
        self
    }

    /// Sets the size of the buffer used to hold the next block that Zopfli will compress. The
    /// larger the buffer, the more effective the compression, but the more memory is required.
    /// A value of `None` indicates no buffer, which is recommended only when all non-empty writes
    /// are larger than about 32 KiB.
    #[must_use]
    #[cfg(feature = "deflate-zopfli")]
    pub const fn with_zopfli_buffer(mut self, size: Option<usize>) -> Self {
        self.zopfli_buffer_size = size;
        self
    }

    /// Returns the compression level currently set.
    pub const fn get_compression_level(&self) -> Option<i64> {
        self.compression_level
    }
    /// Sets the alignment to the given number of bytes.
    #[must_use]
    pub const fn with_alignment(mut self, alignment: u16) -> Self {
        self.alignment = alignment;
        self
    }
}
impl FileOptions<ExtendedFileOptions> {
    /// Adds an extra data field.
    pub fn add_extra_data(
        &mut self,
        header_id: u16,
        data: &[u8],
        central_only: bool,
    ) -> ZipResult<()> {
        validate_extra_data(header_id, data)?;
        let len = data.len() + 4;
        if self.extended_options.extra_data.len()
            + self.extended_options.central_extra_data.len()
            + len
            > u16::MAX as usize
        {
            Err(InvalidArchive(
                "Extra data field would be longer than allowed",
            ))
        } else {
            let field = if central_only {
                &mut self.extended_options.central_extra_data
            } else {
                &mut self.extended_options.extra_data
            };
            let vec = Arc::get_mut(field);
            let vec = match vec {
                Some(exclusive) => exclusive,
                None => {
                    *field = Arc::new(field.to_vec());
                    Arc::get_mut(field).unwrap()
                }
            };
            vec.reserve_exact(data.len() + 4);
            vec.write_u16_le(header_id)?;
            vec.write_u16_le(data.len() as u16)?;
            vec.write_all(data)?;
            Ok(())
        }
    }

    /// Removes the extra data fields.
    #[must_use]
    pub fn clear_extra_data(mut self) -> Self {
        if self.extended_options.extra_data.len() > 0 {
            self.extended_options.extra_data = Arc::new(vec![]);
        }
        if self.extended_options.central_extra_data.len() > 0 {
            self.extended_options.central_extra_data = Arc::new(vec![]);
        }
        self
    }
}
impl<T: FileOptionExtension> Default for FileOptions<T> {
    /// Construct a new FileOptions object
    fn default() -> Self {
        Self {
            compression_method: Default::default(),
            compression_level: None,
            #[cfg(feature = "time")]
            last_modified_time: OffsetDateTime::now_utc().try_into().unwrap_or_default(),
            #[cfg(not(feature = "time"))]
            last_modified_time: DateTime::default(),
            permissions: None,
            large_file: false,
            encrypt_with: None,
            extended_options: T::default(),
            alignment: 1,
            #[cfg(feature = "deflate-zopfli")]
            zopfli_buffer_size: Some(1 << 15),
        }
    }
}

impl<W: Write + Seek> Write for ZipWriter<W> {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        if !self.writing_to_file {
            return Err(io::Error::new(
                io::ErrorKind::Other,
                "No file has been started",
            ));
        }
        if buf.is_empty() {
            return Ok(0);
        }
        match self.inner.ref_mut() {
            Some(ref mut w) => {
                let write_result = w.write(buf);
                if let Ok(count) = write_result {
                    self.stats.update(&buf[0..count]);
                    if self.stats.bytes_written > spec::ZIP64_BYTES_THR
                        && !self.files.last_mut().unwrap().1.large_file
                    {
                        self.abort_file().unwrap();
                        return Err(io::Error::new(
                            io::ErrorKind::Other,
                            "Large file option has not been set",
                        ));
                    }
                }
                write_result
            }
            None => Err(io::Error::new(
                io::ErrorKind::BrokenPipe,
                "write(): ZipWriter was already closed",
            )),
        }
    }

    fn flush(&mut self) -> io::Result<()> {
        match self.inner.ref_mut() {
            Some(ref mut w) => w.flush(),
            None => Err(io::Error::new(
                io::ErrorKind::BrokenPipe,
                "flush(): ZipWriter was already closed",
            )),
        }
    }
}

impl ZipWriterStats {
    fn update(&mut self, buf: &[u8]) {
        self.hasher.update(buf);
        self.bytes_written += buf.len() as u64;
    }
}

impl<A: Read + Write + Seek> ZipWriter<A> {
    /// Initializes the archive from an existing ZIP archive, making it ready for append.
    pub fn new_append(mut readwriter: A) -> ZipResult<ZipWriter<A>> {
        let (footer, cde_start_pos) = spec::CentralDirectoryEnd::find_and_parse(&mut readwriter)?;
        let metadata = ZipArchive::get_metadata(&mut readwriter, &footer, cde_start_pos)?;

        Ok(ZipWriter {
            inner: Storer(MaybeEncrypted::Unencrypted(readwriter)),
            files: metadata.files,
            stats: Default::default(),
            writing_to_file: false,
            comment: footer.zip_file_comment,
            writing_raw: true, // avoid recomputing the last file's header
            flush_on_finish_file: false,
        })
    }

    /// `flush_on_finish_file` is designed to support a streaming `inner` that may unload flushed
    /// bytes. It flushes a file's header and body once it starts writing another file. A ZipWriter
    /// will not try to seek back into where a previous file was written unless
    /// either [`ZipWriter::abort_file`] is called while [`ZipWriter::is_writing_file`] returns
    /// false, or [`ZipWriter::deep_copy_file`] is called. In the latter case, it will only need to
    /// read previously-written files and not overwrite them.
    ///
    /// Note: when using an `inner` that cannot overwrite flushed bytes, do not wrap it in a
    /// [std::io::BufWriter], because that has a [Seek::seek] method that implicitly calls
    /// [BufWriter::flush], and ZipWriter needs to seek backward to update each file's header with
    /// the size and checksum after writing the body.
    ///
    /// This setting is false by default.
    pub fn set_flush_on_finish_file(&mut self, flush_on_finish_file: bool) {
        self.flush_on_finish_file = flush_on_finish_file;
    }
}

impl<A: Read + Write + Seek> ZipWriter<A> {
    /// Adds another copy of a file already in this archive. This will produce a larger but more
    /// widely-compatible archive compared to [Self::shallow_copy_file]. Does not copy alignment.
    pub fn deep_copy_file(&mut self, src_name: &str, dest_name: &str) -> ZipResult<()> {
        self.finish_file()?;
        let write_position = self.inner.get_plain().stream_position()?;
        let src_index = self.index_by_name(src_name)?;
        let src_data = &self.files[src_index];
        let data_start = *src_data.data_start.get().unwrap_or(&0);
        let compressed_size = src_data.compressed_size;
        debug_assert!(compressed_size <= write_position - data_start);
        let uncompressed_size = src_data.uncompressed_size;

        let raw_values = ZipRawValues {
            crc32: src_data.crc32,
            compressed_size,
            uncompressed_size,
        };
        let mut reader = BufReader::new(ZipFileReader::Raw(find_content(
            src_data,
            self.inner.get_plain(),
        )?));
        let mut copy = Vec::with_capacity(compressed_size as usize);
        reader.read_to_end(&mut copy)?;
        drop(reader);
        self.inner
            .get_plain()
            .seek(SeekFrom::Start(write_position))?;
        if src_data.extra_field.is_some() || src_data.central_extra_field.is_some() {
            let mut options = FileOptions::<ExtendedFileOptions> {
                compression_method: src_data.compression_method,
                compression_level: src_data.compression_level,
                last_modified_time: src_data.last_modified_time,
                permissions: src_data.unix_mode(),
                large_file: src_data.large_file,
                encrypt_with: None,
                extended_options: ExtendedFileOptions {
                    extra_data: src_data.extra_field.clone().unwrap_or_default(),
                    central_extra_data: src_data.central_extra_field.clone().unwrap_or_default(),
                },
                alignment: 1,
                #[cfg(feature = "deflate-zopfli")]
                zopfli_buffer_size: None,
            };
            if let Some(perms) = src_data.unix_mode() {
                options = options.unix_permissions(perms);
            }
            Self::normalize_options(&mut options);
            self.start_entry(dest_name, options, Some(raw_values))?;
        } else {
            let mut options = FileOptions::<()> {
                compression_method: src_data.compression_method,
                compression_level: src_data.compression_level,
                last_modified_time: src_data.last_modified_time,
                permissions: src_data.unix_mode(),
                large_file: src_data.large_file,
                encrypt_with: None,
                extended_options: (),
                alignment: 1,
                #[cfg(feature = "deflate-zopfli")]
                zopfli_buffer_size: None,
            };
            if let Some(perms) = src_data.unix_mode() {
                options = options.unix_permissions(perms);
            }
            Self::normalize_options(&mut options);
            self.start_entry(dest_name, options, Some(raw_values))?;
        }

        self.writing_to_file = true;
        self.writing_raw = true;
        if let Err(e) = self.write_all(&copy) {
            self.abort_file().unwrap();
            return Err(e.into());
        }
        self.finish_file()
    }

    /// Like `deep_copy_file`, but uses Path arguments.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn deep_copy_file_from_path<T: AsRef<Path>, U: AsRef<Path>>(
        &mut self,
        src_path: T,
        dest_path: U,
    ) -> ZipResult<()> {
        self.deep_copy_file(&path_to_string(src_path), &path_to_string(dest_path))
    }

    /// Write the zip file into the backing stream, then produce a readable archive of that data.
    ///
    /// This method avoids parsing the central directory records at the end of the stream for
    /// a slight performance improvement over running [`ZipArchive::new()`] on the output of
    /// [`Self::finish()`].
    ///
    ///```
    /// # fn main() -> Result<(), zip::result::ZipError> {
    /// use std::io::{Cursor, prelude::*};
    /// use zip::{ZipArchive, ZipWriter, write::SimpleFileOptions};
    ///
    /// let buf = Cursor::new(Vec::new());
    /// let mut zip = ZipWriter::new(buf);
    /// let options = SimpleFileOptions::default();
    /// zip.start_file("a.txt", options)?;
    /// zip.write_all(b"hello\n")?;
    ///
    /// let mut zip = zip.finish_into_readable()?;
    /// let mut s: String = String::new();
    /// zip.by_name("a.txt")?.read_to_string(&mut s)?;
    /// assert_eq!(s, "hello\n");
    /// # Ok(())
    /// # }
    ///```
    pub fn finish_into_readable(mut self) -> ZipResult<ZipArchive<A>> {
        let central_start = self.finalize()?;
        let inner = mem::replace(&mut self.inner, Closed).unwrap();
        let comment = mem::take(&mut self.comment);
        let files = mem::take(&mut self.files);
        let archive = ZipArchive::from_finalized_writer(files, comment, inner, central_start)?;
        Ok(archive)
    }
}

impl<W: Write + Seek> ZipWriter<W> {
    /// Initializes the archive.
    ///
    /// Before writing to this object, the [`ZipWriter::start_file`] function should be called.
    /// After a successful write, the file remains open for writing. After a failed write, call
    /// [`ZipWriter::is_writing_file`] to determine if the file remains open.
    pub fn new(inner: W) -> ZipWriter<W> {
        ZipWriter {
            inner: Storer(MaybeEncrypted::Unencrypted(inner)),
            files: IndexMap::new(),
            stats: Default::default(),
            writing_to_file: false,
            writing_raw: false,
            comment: Vec::new(),
            flush_on_finish_file: false,
        }
    }

    /// Returns true if a file is currently open for writing.
    pub const fn is_writing_file(&self) -> bool {
        self.writing_to_file && !self.inner.is_closed()
    }

    /// Set ZIP archive comment.
    pub fn set_comment<S>(&mut self, comment: S)
    where
        S: Into<String>,
    {
        self.set_raw_comment(comment.into().into())
    }

    /// Set ZIP archive comment.
    ///
    /// This sets the raw bytes of the comment. The comment
    /// is typically expected to be encoded in UTF-8
    pub fn set_raw_comment(&mut self, comment: Vec<u8>) {
        self.comment = comment;
    }

    /// Get ZIP archive comment.
    pub fn get_comment(&mut self) -> Result<&str, Utf8Error> {
        from_utf8(self.get_raw_comment())
    }

    /// Get ZIP archive comment.
    ///
    /// This returns the raw bytes of the comment. The comment
    /// is typically expected to be encoded in UTF-8
    pub const fn get_raw_comment(&self) -> &Vec<u8> {
        &self.comment
    }

    /// Start a new file for with the requested options.
    fn start_entry<S, T: FileOptionExtension>(
        &mut self,
        name: S,
        options: FileOptions<T>,
        raw_values: Option<ZipRawValues>,
    ) -> ZipResult<()>
    where
        S: Into<Box<str>>,
    {
        self.finish_file()?;

        let raw_values = raw_values.unwrap_or(ZipRawValues {
            crc32: 0,
            compressed_size: 0,
            uncompressed_size: 0,
        });

        {
            let header_start = self.inner.get_plain().stream_position()?;

            let permissions = options.permissions.unwrap_or(0o100644);
            let file = ZipFileData {
                system: System::Unix,
                version_made_by: DEFAULT_VERSION,
                encrypted: options.encrypt_with.is_some(),
                using_data_descriptor: false,
                compression_method: options.compression_method,
                compression_level: options.compression_level,
                last_modified_time: options.last_modified_time,
                crc32: raw_values.crc32,
                compressed_size: raw_values.compressed_size,
                uncompressed_size: raw_values.uncompressed_size,
                file_name: name.into(),
                file_name_raw: vec![].into_boxed_slice(), // Never used for saving
                extra_field: options.extended_options.extra_data().cloned(),
                central_extra_field: options.extended_options.central_extra_data().cloned(),
                file_comment: String::with_capacity(0).into_boxed_str(),
                header_start,
                data_start: OnceLock::new(),
                central_header_start: 0,
                external_attributes: permissions << 16,
                large_file: options.large_file,
                aes_mode: None,
                extra_fields: Vec::new(),
            };
            let index = self.insert_file_data(file)?;
            let file = &mut self.files[index];
            let writer = self.inner.get_plain();
            writer.write_u32_le(spec::LOCAL_FILE_HEADER_SIGNATURE)?;
            // version needed to extract
            writer.write_u16_le(file.version_needed())?;
            // general purpose bit flag
            let flag = if !file.file_name.is_ascii() {
                1u16 << 11
            } else {
                0
            } | if file.encrypted { 1u16 << 0 } else { 0 };
            writer.write_u16_le(flag)?;
            // Compression method
            #[allow(deprecated)]
            writer.write_u16_le(file.compression_method.to_u16())?;
            // last mod file time and last mod file date
            writer.write_u16_le(file.last_modified_time.timepart())?;
            writer.write_u16_le(file.last_modified_time.datepart())?;
            // crc-32
            writer.write_u32_le(file.crc32)?;
            // compressed size and uncompressed size
            if file.large_file {
                writer.write_u32_le(spec::ZIP64_BYTES_THR as u32)?;
                writer.write_u32_le(spec::ZIP64_BYTES_THR as u32)?;
            } else {
                writer.write_u32_le(file.compressed_size as u32)?;
                writer.write_u32_le(file.uncompressed_size as u32)?;
            }
            // file name length
            writer.write_u16_le(file.file_name.as_bytes().len() as u16)?;
            // extra field length
            let mut extra_field_length = file.extra_field_len();
            if file.large_file {
                extra_field_length += 20;
            }
            if extra_field_length + file.central_extra_field_len() > u16::MAX as usize {
                let _ = self.abort_file();
                return Err(InvalidArchive("Extra data field is too large"));
            }
            let extra_field_length = extra_field_length as u16;
            writer.write_u16_le(extra_field_length)?;
            // file name
            writer.write_all(file.file_name.as_bytes())?;
            // zip64 extra field
            if file.large_file {
                write_local_zip64_extra_field(writer, file)?;
            }
            if let Some(extra_field) = &file.extra_field {
                writer.write_all(extra_field)?;
            }
            let mut header_end = writer.stream_position()?;
            if options.alignment > 1 {
                let align = options.alignment as u64;
                let unaligned_header_bytes = header_end % align;
                if unaligned_header_bytes != 0 {
                    let pad_length = (align - unaligned_header_bytes) as usize;
                    let Some(new_extra_field_length) =
                        (pad_length as u16).checked_add(extra_field_length)
                    else {
                        let _ = self.abort_file();
                        return Err(InvalidArchive(
                            "Extra data field would be larger than allowed after aligning",
                        ));
                    };
                    if pad_length >= 4 {
                        // Add an extra field to the extra_data
                        let pad_body = vec![0; pad_length - 4];
                        writer.write_all(b"za").map_err(ZipError::from)?; // 0x617a
                        writer
                            .write_u16_le(pad_body.len() as u16)
                            .map_err(ZipError::from)?;
                        writer.write_all(&pad_body).map_err(ZipError::from)?;
                    } else {
                        // extra_data padding is too small for an extra field header, so pad with
                        // zeroes
                        let pad = vec![0; pad_length];
                        writer.write_all(&pad).map_err(ZipError::from)?;
                    }
                    header_end = writer.stream_position()?;

                    // Update extra field length in local file header.
                    writer.seek(SeekFrom::Start(file.header_start + 28))?;
                    writer.write_u16_le(new_extra_field_length)?;
                    writer.seek(SeekFrom::Start(header_end))?;
                    debug_assert_eq!(header_end % align, 0);
                }
            }
            if let Some(keys) = options.encrypt_with {
                let mut zipwriter = crate::zipcrypto::ZipCryptoWriter {
                    writer: mem::replace(&mut self.inner, Closed).unwrap(),
                    buffer: vec![],
                    keys,
                };
                let crypto_header = [0u8; 12];

                zipwriter.write_all(&crypto_header)?;
                header_end = zipwriter.writer.stream_position()?;
                self.inner = Storer(MaybeEncrypted::Encrypted(zipwriter));
            }
            self.stats.start = header_end;
            debug_assert!(file.data_start.get().is_none());
            file.data_start.get_or_init(|| header_end);
            self.writing_to_file = true;
            self.stats.bytes_written = 0;
            self.stats.hasher = Hasher::new();
        }
        Ok(())
    }

    fn insert_file_data(&mut self, file: ZipFileData) -> ZipResult<usize> {
        if self.files.contains_key(&file.file_name) {
            return Err(InvalidArchive("Duplicate filename"));
        }
        let name = file.file_name.to_owned();
        self.files.insert(name.clone(), file);
        Ok(self.files.get_index_of(&name).unwrap())
    }

    fn finish_file(&mut self) -> ZipResult<()> {
        if !self.writing_to_file {
            return Ok(());
        }

        let make_plain_writer = self.inner.prepare_next_writer(
            Stored,
            None,
            #[cfg(feature = "deflate-zopfli")]
            None,
        )?;
        self.inner.switch_to(make_plain_writer)?;
        self.switch_to_non_encrypting_writer()?;
        let writer = self.inner.get_plain();

        if !self.writing_raw {
            let file = match self.files.last_mut() {
                None => return Ok(()),
                Some((_, f)) => f,
            };
            file.crc32 = self.stats.hasher.clone().finalize();
            file.uncompressed_size = self.stats.bytes_written;

            let file_end = writer.stream_position()?;
            debug_assert!(file_end >= self.stats.start);
            file.compressed_size = file_end - self.stats.start;

            update_local_file_header(writer, file)?;
            writer.seek(SeekFrom::Start(file_end))?;
        }
        if self.flush_on_finish_file {
            if let Err(e) = writer.flush() {
                self.abort_file()?;
                return Err(e.into());
            }
        }

        self.writing_to_file = false;
        Ok(())
    }

    fn switch_to_non_encrypting_writer(&mut self) -> Result<(), ZipError> {
        match mem::replace(&mut self.inner, Closed) {
            Storer(MaybeEncrypted::Encrypted(writer)) => {
                let crc32 = self.stats.hasher.clone().finalize();
                self.inner = Storer(MaybeEncrypted::Unencrypted(writer.finish(crc32)?))
            }
            Storer(MaybeEncrypted::Unencrypted(w)) => {
                self.inner = Storer(MaybeEncrypted::Unencrypted(w))
            }
            _ => unreachable!(),
        }
        Ok(())
    }

    /// Removes the file currently being written from the archive if there is one, or else removes
    /// the file most recently written.
    pub fn abort_file(&mut self) -> ZipResult<()> {
        let (_, last_file) = self.files.pop().ok_or(ZipError::FileNotFound)?;
        let make_plain_writer = self.inner.prepare_next_writer(
            Stored,
            None,
            #[cfg(feature = "deflate-zopfli")]
            None,
        )?;
        self.inner.switch_to(make_plain_writer)?;
        self.switch_to_non_encrypting_writer()?;
        // Make sure this is the last file, and that no shallow copies of it remain; otherwise we'd
        // overwrite a valid file and corrupt the archive
        let rewind_safe: bool = match last_file.data_start.get() {
            None => self.files.is_empty(),
            Some(last_file_start) => self.files.values().all(|file| {
                file.data_start
                    .get()
                    .is_some_and(|start| start < last_file_start)
            }),
        };
        if rewind_safe {
            self.inner
                .get_plain()
                .seek(SeekFrom::Start(last_file.header_start))?;
        }
        self.writing_to_file = false;
        Ok(())
    }

    /// Create a file in the archive and start writing its' contents. The file must not have the
    /// same name as a file already in the archive.
    ///
    /// The data should be written using the [`Write`] implementation on this [`ZipWriter`]
    pub fn start_file<S, T: FileOptionExtension>(
        &mut self,
        name: S,
        mut options: FileOptions<T>,
    ) -> ZipResult<()>
    where
        S: Into<Box<str>>,
    {
        Self::normalize_options(&mut options);
        let make_new_self = self.inner.prepare_next_writer(
            options.compression_method,
            options.compression_level,
            #[cfg(feature = "deflate-zopfli")]
            options.zopfli_buffer_size,
        )?;
        self.start_entry(name, options, None)?;
        if let Err(e) = self.inner.switch_to(make_new_self) {
            self.abort_file().unwrap();
            return Err(e);
        }
        self.writing_raw = false;
        Ok(())
    }

    /* TODO: link to/use Self::finish_into_readable() from https://github.com/zip-rs/zip/pull/400 in
     * this docstring. */
    /// Copy over the entire contents of another archive verbatim.
    ///
    /// This method extracts file metadata from the `source` archive, then simply performs a single
    /// big [`io::copy()`](io::copy) to transfer all the actual file contents without any
    /// decompression or decryption. This is more performant than the equivalent operation of
    /// calling [`Self::raw_copy_file()`] for each entry from the `source` archive in sequence.
    ///
    ///```
    /// # fn main() -> Result<(), zip::result::ZipError> {
    /// use std::io::{Cursor, prelude::*};
    /// use zip::{ZipArchive, ZipWriter, write::SimpleFileOptions};
    ///
    /// let buf = Cursor::new(Vec::new());
    /// let mut zip = ZipWriter::new(buf);
    /// zip.start_file("a.txt", SimpleFileOptions::default())?;
    /// zip.write_all(b"hello\n")?;
    /// let src = ZipArchive::new(zip.finish()?)?;
    ///
    /// let buf = Cursor::new(Vec::new());
    /// let mut zip = ZipWriter::new(buf);
    /// zip.start_file("b.txt", SimpleFileOptions::default())?;
    /// zip.write_all(b"hey\n")?;
    /// let src2 = ZipArchive::new(zip.finish()?)?;
    ///
    /// let buf = Cursor::new(Vec::new());
    /// let mut zip = ZipWriter::new(buf);
    /// zip.merge_archive(src)?;
    /// zip.merge_archive(src2)?;
    /// let mut result = ZipArchive::new(zip.finish()?)?;
    ///
    /// let mut s: String = String::new();
    /// result.by_name("a.txt")?.read_to_string(&mut s)?;
    /// assert_eq!(s, "hello\n");
    /// s.clear();
    /// result.by_name("b.txt")?.read_to_string(&mut s)?;
    /// assert_eq!(s, "hey\n");
    /// # Ok(())
    /// # }
    ///```
    pub fn merge_archive<R>(&mut self, mut source: ZipArchive<R>) -> ZipResult<()>
    where
        R: Read + io::Seek,
    {
        self.finish_file()?;

        /* Ensure we accept the file contents on faith (and avoid overwriting the data).
         * See raw_copy_file_rename(). */
        self.writing_to_file = true;
        self.writing_raw = true;

        let writer = self.inner.get_plain();
        /* Get the file entries from the source archive. */
        let new_files = source.merge_contents(writer)?;

        /* These file entries are now ours! */
        self.files.extend(new_files);

        Ok(())
    }

    fn normalize_options<T: FileOptionExtension>(options: &mut FileOptions<T>) {
        if options.permissions.is_none() {
            options.permissions = Some(0o644);
        }
        if !options.last_modified_time.is_valid() {
            options.last_modified_time = FileOptions::<T>::default().last_modified_time;
        }
        *options.permissions.as_mut().unwrap() |= ffi::S_IFREG;
    }

    /// Starts a file, taking a Path as argument.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn start_file_from_path<E: FileOptionExtension, P: AsRef<Path>>(
        &mut self,
        path: P,
        options: FileOptions<E>,
    ) -> ZipResult<()> {
        self.start_file(path_to_string(path), options)
    }

    /// Add a new file using the already compressed data from a ZIP file being read and renames it, this
    /// allows faster copies of the `ZipFile` since there is no need to decompress and compress it again.
    /// Any `ZipFile` metadata is copied and not checked, for example the file CRC.

    /// ```no_run
    /// use std::fs::File;
    /// use std::io::{Read, Seek, Write};
    /// use zip::{ZipArchive, ZipWriter};
    ///
    /// fn copy_rename<R, W>(
    ///     src: &mut ZipArchive<R>,
    ///     dst: &mut ZipWriter<W>,
    /// ) -> zip::result::ZipResult<()>
    /// where
    ///     R: Read + Seek,
    ///     W: Write + Seek,
    /// {
    ///     // Retrieve file entry by name
    ///     let file = src.by_name("src_file.txt")?;
    ///
    ///     // Copy and rename the previously obtained file entry to the destination zip archive
    ///     dst.raw_copy_file_rename(file, "new_name.txt")?;
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn raw_copy_file_rename<S>(&mut self, mut file: ZipFile, name: S) -> ZipResult<()>
    where
        S: Into<Box<str>>,
    {
        let mut options = SimpleFileOptions::default()
            .large_file(file.compressed_size().max(file.size()) > spec::ZIP64_BYTES_THR)
            .last_modified_time(file.last_modified())
            .compression_method(file.compression());
        if let Some(perms) = file.unix_mode() {
            options = options.unix_permissions(perms);
        }
        Self::normalize_options(&mut options);

        let raw_values = ZipRawValues {
            crc32: file.crc32(),
            compressed_size: file.compressed_size(),
            uncompressed_size: file.size(),
        };

        self.start_entry(name, options, Some(raw_values))?;
        self.writing_to_file = true;
        self.writing_raw = true;

        io::copy(file.get_raw_reader(), self)?;

        Ok(())
    }

    /// Like `raw_copy_file_to_path`, but uses Path arguments.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn raw_copy_file_to_path<P: AsRef<Path>>(
        &mut self,
        file: ZipFile,
        path: P,
    ) -> ZipResult<()> {
        self.raw_copy_file_rename(file, path_to_string(path))
    }

    /// Add a new file using the already compressed data from a ZIP file being read, this allows faster
    /// copies of the `ZipFile` since there is no need to decompress and compress it again. Any `ZipFile`
    /// metadata is copied and not checked, for example the file CRC.
    ///
    /// ```no_run
    /// use std::fs::File;
    /// use std::io::{Read, Seek, Write};
    /// use zip::{ZipArchive, ZipWriter};
    ///
    /// fn copy<R, W>(src: &mut ZipArchive<R>, dst: &mut ZipWriter<W>) -> zip::result::ZipResult<()>
    /// where
    ///     R: Read + Seek,
    ///     W: Write + Seek,
    /// {
    ///     // Retrieve file entry by name
    ///     let file = src.by_name("src_file.txt")?;
    ///
    ///     // Copy the previously obtained file entry to the destination zip archive
    ///     dst.raw_copy_file(file)?;
    ///
    ///     Ok(())
    /// }
    /// ```
    pub fn raw_copy_file(&mut self, file: ZipFile) -> ZipResult<()> {
        let name = file.name().to_owned();
        self.raw_copy_file_rename(file, name)
    }

    /// Add a directory entry.
    ///
    /// As directories have no content, you must not call [`ZipWriter::write`] before adding a new file.
    pub fn add_directory<S, T: FileOptionExtension>(
        &mut self,
        name: S,
        mut options: FileOptions<T>,
    ) -> ZipResult<()>
    where
        S: Into<String>,
    {
        if options.permissions.is_none() {
            options.permissions = Some(0o755);
        }
        *options.permissions.as_mut().unwrap() |= 0o40000;
        options.compression_method = Stored;

        let name_as_string = name.into();
        // Append a slash to the filename if it does not end with it.
        let name_with_slash = match name_as_string.chars().last() {
            Some('/') | Some('\\') => name_as_string,
            _ => name_as_string + "/",
        };

        self.start_entry(name_with_slash, options, None)?;
        self.writing_to_file = false;
        self.switch_to_non_encrypting_writer()?;
        Ok(())
    }

    /// Add a directory entry, taking a Path as argument.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn add_directory_from_path<T: FileOptionExtension, P: AsRef<Path>>(
        &mut self,
        path: P,
        options: FileOptions<T>,
    ) -> ZipResult<()> {
        self.add_directory(path_to_string(path), options)
    }

    /// Finish the last file and write all other zip-structures
    ///
    /// This will return the writer, but one should normally not append any data to the end of the file.
    /// Note that the zipfile will also be finished on drop.
    pub fn finish(&mut self) -> ZipResult<W> {
        let _central_start = self.finalize()?;
        let inner = mem::replace(&mut self.inner, Closed);
        Ok(inner.unwrap())
    }

    /// Add a symlink entry.
    ///
    /// The zip archive will contain an entry for path `name` which is a symlink to `target`.
    ///
    /// No validation or normalization of the paths is performed. For best results,
    /// callers should normalize `\` to `/` and ensure symlinks are relative to other
    /// paths within the zip archive.
    ///
    /// WARNING: not all zip implementations preserve symlinks on extract. Some zip
    /// implementations may materialize a symlink as a regular file, possibly with the
    /// content incorrectly set to the symlink target. For maximum portability, consider
    /// storing a regular file instead.
    pub fn add_symlink<N, T, E: FileOptionExtension>(
        &mut self,
        name: N,
        target: T,
        mut options: FileOptions<E>,
    ) -> ZipResult<()>
    where
        N: Into<Box<str>>,
        T: Into<Box<str>>,
    {
        if options.permissions.is_none() {
            options.permissions = Some(0o777);
        }
        *options.permissions.as_mut().unwrap() |= 0o120000;
        // The symlink target is stored as file content. And compressing the target path
        // likely wastes space. So always store.
        options.compression_method = Stored;

        self.start_entry(name, options, None)?;
        self.writing_to_file = true;
        if let Err(e) = self.write_all(target.into().as_bytes()) {
            self.abort_file().unwrap();
            return Err(e.into());
        }
        self.finish_file()?;

        Ok(())
    }

    /// Add a symlink entry, taking Paths to the location and target as arguments.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn add_symlink_from_path<P: AsRef<Path>, T: AsRef<Path>, E: FileOptionExtension>(
        &mut self,
        path: P,
        target: T,
        options: FileOptions<E>,
    ) -> ZipResult<()> {
        self.add_symlink(path_to_string(path), path_to_string(target), options)
    }

    fn finalize(&mut self) -> ZipResult<u64> {
        self.finish_file()?;

        let central_start = {
            let central_start = self.write_central_and_footer()?;
            let writer = self.inner.get_plain();
            let footer_end = writer.stream_position()?;
            let file_end = writer.seek(SeekFrom::End(0))?;
            if footer_end < file_end {
                // Data from an aborted file is past the end of the footer, so rewrite the footer at
                // the actual end.
                let central_and_footer_size = footer_end - central_start;
                writer.seek(SeekFrom::End(-(central_and_footer_size as i64)))?;
                self.write_central_and_footer()?;
            }
            central_start
        };

        Ok(central_start)
    }

    fn write_central_and_footer(&mut self) -> Result<u64, ZipError> {
        let writer = self.inner.get_plain();

        let central_start = writer.stream_position()?;
        for file in self.files.values() {
            write_central_directory_header(writer, file)?;
        }
        let central_size = writer.stream_position()? - central_start;

        if self.files.len() > spec::ZIP64_ENTRY_THR
            || central_size.max(central_start) > spec::ZIP64_BYTES_THR
        {
            let zip64_footer = spec::Zip64CentralDirectoryEnd {
                version_made_by: DEFAULT_VERSION as u16,
                version_needed_to_extract: DEFAULT_VERSION as u16,
                disk_number: 0,
                disk_with_central_directory: 0,
                number_of_files_on_this_disk: self.files.len() as u64,
                number_of_files: self.files.len() as u64,
                central_directory_size: central_size,
                central_directory_offset: central_start,
            };

            zip64_footer.write(writer)?;

            let zip64_footer = spec::Zip64CentralDirectoryEndLocator {
                disk_with_central_directory: 0,
                end_of_central_directory_offset: central_start + central_size,
                number_of_disks: 1,
            };

            zip64_footer.write(writer)?;
        }

        let number_of_files = self.files.len().min(spec::ZIP64_ENTRY_THR) as u16;
        let footer = spec::CentralDirectoryEnd {
            disk_number: 0,
            disk_with_central_directory: 0,
            zip_file_comment: self.comment.clone(),
            number_of_files_on_this_disk: number_of_files,
            number_of_files,
            central_directory_size: central_size.min(spec::ZIP64_BYTES_THR) as u32,
            central_directory_offset: central_start.min(spec::ZIP64_BYTES_THR) as u32,
        };

        footer.write(writer)?;
        Ok(central_start)
    }

    fn index_by_name(&self, name: &str) -> ZipResult<usize> {
        self.files.get_index_of(name).ok_or(ZipError::FileNotFound)
    }

    /// Adds another entry to the central directory referring to the same content as an existing
    /// entry. The file's local-file header will still refer to it by its original name, so
    /// unzipping the file will technically be unspecified behavior. [ZipArchive] ignores the
    /// filename in the local-file header and treat the central directory as authoritative. However,
    /// some other software (e.g. Minecraft) will refuse to extract a file copied this way.
    pub fn shallow_copy_file(&mut self, src_name: &str, dest_name: &str) -> ZipResult<()> {
        self.finish_file()?;
        let src_index = self.index_by_name(src_name)?;
        let mut dest_data = self.files[src_index].to_owned();
        dest_data.file_name = dest_name.into();
        self.insert_file_data(dest_data)?;
        Ok(())
    }

    /// Like `shallow_copy_file`, but uses Path arguments.
    ///
    /// This function ensures that the '/' path separator is used and normalizes `.` and `..`. It
    /// ignores any `..` or Windows drive letter that would produce a path outside the ZIP file's
    /// root.
    pub fn shallow_copy_file_from_path<T: AsRef<Path>, U: AsRef<Path>>(
        &mut self,
        src_path: T,
        dest_path: U,
    ) -> ZipResult<()> {
        self.shallow_copy_file(&path_to_string(src_path), &path_to_string(dest_path))
    }
}

impl<W: Write + Seek> Drop for ZipWriter<W> {
    fn drop(&mut self) {
        if !self.inner.is_closed() {
            if let Err(e) = self.finalize() {
                let _ = write!(io::stderr(), "ZipWriter drop failed: {:?}", e);
            }
        }
    }
}

type SwitchWriterFunction<W> = Box<dyn FnOnce(MaybeEncrypted<W>) -> GenericZipWriter<W>>;

impl<W: Write + Seek> GenericZipWriter<W> {
    fn prepare_next_writer(
        &self,
        compression: CompressionMethod,
        compression_level: Option<i64>,
        #[cfg(feature = "deflate-zopfli")] zopfli_buffer_size: Option<usize>,
    ) -> ZipResult<SwitchWriterFunction<W>> {
        if let Closed = self {
            return Err(
                io::Error::new(io::ErrorKind::BrokenPipe, "ZipWriter was already closed").into(),
            );
        }

        {
            #[allow(deprecated)]
            #[allow(unreachable_code)]
            match compression {
                Stored => {
                    if compression_level.is_some() {
                        Err(ZipError::UnsupportedArchive(
                            "Unsupported compression level",
                        ))
                    } else {
                        Ok(Box::new(|bare| Storer(bare)))
                    }
                }
                #[cfg(feature = "_deflate-any")]
                CompressionMethod::Deflated => {
                    let default = if cfg!(feature = "deflate-zopfli") {
                        24
                    } else {
                        Compression::default().level() as i64
                    };

                    let level = clamp_opt(
                        compression_level.unwrap_or(default),
                        deflate_compression_level_range(),
                    )
                    .ok_or(ZipError::UnsupportedArchive(
                        "Unsupported compression level",
                    ))? as u32;

                    #[cfg(feature = "deflate-zopfli")]
                    {
                        let best_non_zopfli = Compression::best().level();
                        if level > best_non_zopfli {
                            let options = Options {
                                iteration_count: NonZeroU64::try_from(
                                    (level - best_non_zopfli) as u64,
                                )
                                .unwrap(),
                                ..Default::default()
                            };
                            return Ok(Box::new(move |bare| match zopfli_buffer_size {
                                Some(size) => GenericZipWriter::BufferedZopfliDeflater(
                                    BufWriter::with_capacity(
                                        size,
                                        zopfli::DeflateEncoder::new(
                                            options,
                                            Default::default(),
                                            bare,
                                        ),
                                    ),
                                ),
                                None => GenericZipWriter::ZopfliDeflater(
                                    zopfli::DeflateEncoder::new(options, Default::default(), bare),
                                ),
                            }));
                        }
                    }

                    #[cfg(any(
                        feature = "deflate",
                        feature = "deflate-zlib",
                        feature = "deflate-zlib-ng",
                    ))]
                    {
                        return Ok(Box::new(move |bare| {
                            GenericZipWriter::Deflater(DeflateEncoder::new(
                                bare,
                                Compression::new(level),
                            ))
                        }));
                    }
                    unreachable!()
                }
                #[cfg(feature = "deflate64")]
                CompressionMethod::Deflate64 => Err(ZipError::UnsupportedArchive(
                    "Compressing Deflate64 is not supported",
                )),
                #[cfg(feature = "bzip2")]
                CompressionMethod::Bzip2 => {
                    let level = clamp_opt(
                        compression_level.unwrap_or(bzip2::Compression::default().level() as i64),
                        bzip2_compression_level_range(),
                    )
                    .ok_or(ZipError::UnsupportedArchive(
                        "Unsupported compression level",
                    ))? as u32;
                    Ok(Box::new(move |bare| {
                        GenericZipWriter::Bzip2(BzEncoder::new(
                            bare,
                            bzip2::Compression::new(level),
                        ))
                    }))
                }
                CompressionMethod::AES => Err(ZipError::UnsupportedArchive(
                    "AES compression is not supported for writing",
                )),
                #[cfg(feature = "zstd")]
                CompressionMethod::Zstd => {
                    let level = clamp_opt(
                        compression_level.unwrap_or(zstd::DEFAULT_COMPRESSION_LEVEL as i64),
                        zstd::compression_level_range(),
                    )
                    .ok_or(ZipError::UnsupportedArchive(
                        "Unsupported compression level",
                    ))?;
                    Ok(Box::new(move |bare| {
                        GenericZipWriter::Zstd(ZstdEncoder::new(bare, level as i32).unwrap())
                    }))
                }
                #[cfg(feature = "lzma")]
                CompressionMethod::Lzma => {
                    Err(UnsupportedArchive("LZMA isn't supported for compression"))
                }
                CompressionMethod::Unsupported(..) => {
                    Err(ZipError::UnsupportedArchive("Unsupported compression"))
                }
            }
        }
    }

    fn switch_to(&mut self, make_new_self: SwitchWriterFunction<W>) -> ZipResult<()> {
        let bare = match mem::replace(self, Closed) {
            Storer(w) => w,
            #[cfg(any(
                feature = "deflate",
                feature = "deflate-zlib",
                feature = "deflate-zlib-ng"
            ))]
            GenericZipWriter::Deflater(w) => w.finish()?,
            #[cfg(feature = "deflate-zopfli")]
            GenericZipWriter::ZopfliDeflater(w) => w.finish()?,
            #[cfg(feature = "deflate-zopfli")]
            GenericZipWriter::BufferedZopfliDeflater(w) => w
                .into_inner()
                .map_err(|e| ZipError::Io(e.into_error()))?
                .finish()?,
            #[cfg(feature = "bzip2")]
            GenericZipWriter::Bzip2(w) => w.finish()?,
            #[cfg(feature = "zstd")]
            GenericZipWriter::Zstd(w) => w.finish()?,
            Closed => {
                return Err(io::Error::new(
                    io::ErrorKind::BrokenPipe,
                    "ZipWriter was already closed",
                )
                .into());
            }
        };
        *self = make_new_self(bare);
        Ok(())
    }

    fn ref_mut(&mut self) -> Option<&mut dyn Write> {
        match self {
            Storer(ref mut w) => Some(w as &mut dyn Write),
            #[cfg(any(
                feature = "deflate",
                feature = "deflate-zlib",
                feature = "deflate-zlib-ng"
            ))]
            GenericZipWriter::Deflater(ref mut w) => Some(w as &mut dyn Write),
            #[cfg(feature = "deflate-zopfli")]
            GenericZipWriter::ZopfliDeflater(w) => Some(w as &mut dyn Write),
            #[cfg(feature = "deflate-zopfli")]
            GenericZipWriter::BufferedZopfliDeflater(w) => Some(w as &mut dyn Write),
            #[cfg(feature = "bzip2")]
            GenericZipWriter::Bzip2(ref mut w) => Some(w as &mut dyn Write),
            #[cfg(feature = "zstd")]
            GenericZipWriter::Zstd(ref mut w) => Some(w as &mut dyn Write),
            Closed => None,
        }
    }

    const fn is_closed(&self) -> bool {
        matches!(*self, GenericZipWriter::Closed)
    }

    fn get_plain(&mut self) -> &mut W {
        match *self {
            Storer(MaybeEncrypted::Unencrypted(ref mut w)) => w,
            _ => panic!("Should have switched to stored and unencrypted beforehand"),
        }
    }

    fn unwrap(self) -> W {
        match self {
            Storer(MaybeEncrypted::Unencrypted(w)) => w,
            _ => panic!("Should have switched to stored and unencrypted beforehand"),
        }
    }
}

#[cfg(feature = "_deflate-any")]
fn deflate_compression_level_range() -> std::ops::RangeInclusive<i64> {
    let min = if cfg!(feature = "deflate")
        || cfg!(feature = "deflate-zlib")
        || cfg!(feature = "deflate-zlib-ng")
    {
        Compression::none().level() as i64
    } else {
        Compression::best().level() as i64 + 1
    };

    let max = Compression::best().level() as i64
        + if cfg!(feature = "deflate-zopfli") {
            u8::MAX as i64
        } else {
            0
        };

    min..=max
}

#[cfg(feature = "bzip2")]
fn bzip2_compression_level_range() -> std::ops::RangeInclusive<i64> {
    let min = bzip2::Compression::fast().level() as i64;
    let max = bzip2::Compression::best().level() as i64;
    min..=max
}

#[cfg(any(feature = "_deflate-any", feature = "bzip2", feature = "zstd"))]
fn clamp_opt<T: Ord + Copy, U: Ord + Copy + TryFrom<T>>(
    value: T,
    range: std::ops::RangeInclusive<U>,
) -> Option<T> {
    if range.contains(&value.try_into().ok()?) {
        Some(value)
    } else {
        None
    }
}

fn update_local_file_header<T: Write + Seek>(writer: &mut T, file: &ZipFileData) -> ZipResult<()> {
    const CRC32_OFFSET: u64 = 14;
    writer.seek(SeekFrom::Start(file.header_start + CRC32_OFFSET))?;
    writer.write_u32_le(file.crc32)?;
    if file.large_file {
        update_local_zip64_extra_field(writer, file)?;
    } else {
        // check compressed size as well as it can also be slightly larger than uncompressed size
        if file.compressed_size > spec::ZIP64_BYTES_THR {
            return Err(ZipError::Io(io::Error::new(
                io::ErrorKind::Other,
                "Large file option has not been set",
            )));
        }
        writer.write_u32_le(file.compressed_size as u32)?;
        // uncompressed size is already checked on write to catch it as soon as possible
        writer.write_u32_le(file.uncompressed_size as u32)?;
    }
    Ok(())
}

fn write_central_directory_header<T: Write>(writer: &mut T, file: &ZipFileData) -> ZipResult<()> {
    // buffer zip64 extra field to determine its variable length
    let mut zip64_extra_field = [0; 28];
    let zip64_extra_field_length =
        write_central_zip64_extra_field(&mut zip64_extra_field.as_mut(), file)?;

    // central file header signature
    writer.write_u32_le(spec::CENTRAL_DIRECTORY_HEADER_SIGNATURE)?;
    // version made by
    let version_made_by = (file.system as u16) << 8 | (file.version_made_by as u16);
    writer.write_u16_le(version_made_by)?;
    // version needed to extract
    writer.write_u16_le(file.version_needed())?;
    // general purpose bit flag
    let flag = if !file.file_name.is_ascii() {
        1u16 << 11
    } else {
        0
    } | if file.encrypted { 1u16 << 0 } else { 0 };
    writer.write_u16_le(flag)?;
    // compression method
    #[allow(deprecated)]
    writer.write_u16_le(file.compression_method.to_u16())?;
    // last mod file time + date
    writer.write_u16_le(file.last_modified_time.timepart())?;
    writer.write_u16_le(file.last_modified_time.datepart())?;
    // crc-32
    writer.write_u32_le(file.crc32)?;
    // compressed size
    writer.write_u32_le(file.compressed_size.min(spec::ZIP64_BYTES_THR) as u32)?;
    // uncompressed size
    writer.write_u32_le(file.uncompressed_size.min(spec::ZIP64_BYTES_THR) as u32)?;
    // file name length
    writer.write_u16_le(file.file_name.as_bytes().len() as u16)?;
    // extra field length
    writer.write_u16_le(
        zip64_extra_field_length
            + file.extra_field_len() as u16
            + file.central_extra_field_len() as u16,
    )?;
    // file comment length
    writer.write_u16_le(0)?;
    // disk number start
    writer.write_u16_le(0)?;
    // internal file attributes
    writer.write_u16_le(0)?;
    // external file attributes
    writer.write_u32_le(file.external_attributes)?;
    // relative offset of local header
    writer.write_u32_le(file.header_start.min(spec::ZIP64_BYTES_THR) as u32)?;
    // file name
    writer.write_all(file.file_name.as_bytes())?;
    // zip64 extra field
    writer.write_all(&zip64_extra_field[..zip64_extra_field_length as usize])?;
    // extra field
    if let Some(extra_field) = &file.extra_field {
        writer.write_all(extra_field)?;
    }
    if let Some(central_extra_field) = &file.central_extra_field {
        writer.write_all(central_extra_field)?;
    }
    // file comment
    // <none>

    Ok(())
}

fn validate_extra_data(header_id: u16, data: &[u8]) -> ZipResult<()> {
    if data.len() > u16::MAX as usize {
        return Err(ZipError::Io(io::Error::new(
            io::ErrorKind::Other,
            "Extra-data field can't exceed u16::MAX bytes",
        )));
    }
    if header_id == 0x0001 {
        return Err(ZipError::Io(io::Error::new(
            io::ErrorKind::Other,
            "No custom ZIP64 extra data allowed",
        )));
    }

    #[cfg(not(feature = "unreserved"))]
    {
        if header_id <= 31
            || EXTRA_FIELD_MAPPING
                .iter()
                .any(|&mapped| mapped == header_id)
        {
            return Err(ZipError::Io(io::Error::new(
                io::ErrorKind::Other,
                format!(
                    "Extra data header ID {header_id:#06} requires crate feature \"unreserved\"",
                ),
            )));
        }
    }

    Ok(())
}

fn write_local_zip64_extra_field<T: Write>(writer: &mut T, file: &ZipFileData) -> ZipResult<()> {
    // This entry in the Local header MUST include BOTH original
    // and compressed file size fields.
    writer.write_u16_le(0x0001)?;
    writer.write_u16_le(16)?;
    writer.write_u64_le(file.uncompressed_size)?;
    writer.write_u64_le(file.compressed_size)?;
    // Excluded fields:
    // u32: disk start number
    Ok(())
}

fn update_local_zip64_extra_field<T: Write + Seek>(
    writer: &mut T,
    file: &ZipFileData,
) -> ZipResult<()> {
    let zip64_extra_field = file.header_start + 30 + file.file_name.as_bytes().len() as u64;
    writer.seek(SeekFrom::Start(zip64_extra_field + 4))?;
    writer.write_u64_le(file.uncompressed_size)?;
    writer.write_u64_le(file.compressed_size)?;
    // Excluded fields:
    // u32: disk start number
    Ok(())
}

fn write_central_zip64_extra_field<T: Write>(writer: &mut T, file: &ZipFileData) -> ZipResult<u16> {
    // The order of the fields in the zip64 extended
    // information record is fixed, but the fields MUST
    // only appear if the corresponding Local or Central
    // directory record field is set to 0xFFFF or 0xFFFFFFFF.
    let mut size = 0;
    let uncompressed_size = file.uncompressed_size > spec::ZIP64_BYTES_THR;
    let compressed_size = file.compressed_size > spec::ZIP64_BYTES_THR;
    let header_start = file.header_start > spec::ZIP64_BYTES_THR;
    if uncompressed_size {
        size += 8;
    }
    if compressed_size {
        size += 8;
    }
    if header_start {
        size += 8;
    }
    if size > 0 {
        writer.write_u16_le(0x0001)?;
        writer.write_u16_le(size)?;
        size += 4;

        if uncompressed_size {
            writer.write_u64_le(file.uncompressed_size)?;
        }
        if compressed_size {
            writer.write_u64_le(file.compressed_size)?;
        }
        if header_start {
            writer.write_u64_le(file.header_start)?;
        }
        // Excluded fields:
        // u32: disk start number
    }
    Ok(size)
}

#[cfg(not(feature = "unreserved"))]
const EXTRA_FIELD_MAPPING: [u16; 49] = [
    0x0001, 0x0007, 0x0008, 0x0009, 0x000a, 0x000c, 0x000d, 0x000e, 0x000f, 0x0014, 0x0015, 0x0016,
    0x0017, 0x0018, 0x0019, 0x0020, 0x0021, 0x0022, 0x0023, 0x0065, 0x0066, 0x4690, 0x07c8, 0x2605,
    0x2705, 0x2805, 0x334d, 0x4341, 0x4453, 0x4704, 0x470f, 0x4b46, 0x4c41, 0x4d49, 0x4f4c, 0x5356,
    0x5455, 0x554e, 0x5855, 0x6375, 0x6542, 0x7075, 0x756e, 0x7855, 0xa11e, 0xa220, 0xfd4a, 0x9901,
    0x9902,
];

#[cfg(test)]
mod test {
    use super::{FileOptions, ZipWriter};
    use crate::compression::CompressionMethod;
    use crate::result::ZipResult;
    use crate::types::DateTime;
    use crate::write::SimpleFileOptions;
    use crate::CompressionMethod::Stored;
    use crate::ZipArchive;
    use std::io;
    use std::io::{Cursor, Read, Write};
    use std::path::PathBuf;

    #[test]
    fn write_empty_zip() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer.set_comment("ZIP");
        let result = writer.finish().unwrap();
        assert_eq!(result.get_ref().len(), 25);
        assert_eq!(
            *result.get_ref(),
            [80, 75, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 90, 73, 80]
        );
    }

    #[test]
    fn unix_permissions_bitmask() {
        // unix_permissions() throws away upper bits.
        let options = SimpleFileOptions::default().unix_permissions(0o120777);
        assert_eq!(options.permissions, Some(0o777));
    }

    #[test]
    fn write_zip_dir() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .add_directory(
                "test",
                SimpleFileOptions::default().last_modified_time(
                    DateTime::from_date_and_time(2018, 8, 15, 20, 45, 6).unwrap(),
                ),
            )
            .unwrap();
        assert!(writer
            .write(b"writing to a directory is not allowed, and will not write any data")
            .is_err());
        let result = writer.finish().unwrap();
        assert_eq!(result.get_ref().len(), 108);
        assert_eq!(
            *result.get_ref(),
            &[
                80u8, 75, 3, 4, 20, 0, 0, 0, 0, 0, 163, 165, 15, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 5, 0, 0, 0, 116, 101, 115, 116, 47, 80, 75, 1, 2, 46, 3, 20, 0, 0, 0, 0, 0,
                163, 165, 15, 77, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 237, 65, 0, 0, 0, 0, 116, 101, 115, 116, 47, 80, 75, 5, 6, 0, 0, 0, 0, 1, 0,
                1, 0, 51, 0, 0, 0, 35, 0, 0, 0, 0, 0,
            ] as &[u8]
        );
    }

    #[test]
    fn write_symlink_simple() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .add_symlink(
                "name",
                "target",
                SimpleFileOptions::default().last_modified_time(
                    DateTime::from_date_and_time(2018, 8, 15, 20, 45, 6).unwrap(),
                ),
            )
            .unwrap();
        assert!(writer
            .write(b"writing to a symlink is not allowed and will not write any data")
            .is_err());
        let result = writer.finish().unwrap();
        assert_eq!(result.get_ref().len(), 112);
        assert_eq!(
            *result.get_ref(),
            &[
                80u8, 75, 3, 4, 20, 0, 0, 0, 0, 0, 163, 165, 15, 77, 252, 47, 111, 70, 6, 0, 0, 0,
                6, 0, 0, 0, 4, 0, 0, 0, 110, 97, 109, 101, 116, 97, 114, 103, 101, 116, 80, 75, 1,
                2, 46, 3, 20, 0, 0, 0, 0, 0, 163, 165, 15, 77, 252, 47, 111, 70, 6, 0, 0, 0, 6, 0,
                0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 161, 0, 0, 0, 0, 110, 97, 109, 101,
                80, 75, 5, 6, 0, 0, 0, 0, 1, 0, 1, 0, 50, 0, 0, 0, 40, 0, 0, 0, 0, 0
            ] as &[u8],
        );
    }

    #[test]
    fn test_path_normalization() {
        let mut path = PathBuf::new();
        path.push("foo");
        path.push("bar");
        path.push("..");
        path.push(".");
        path.push("example.txt");
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file_from_path(path, SimpleFileOptions::default())
            .unwrap();
        let archive = ZipArchive::new(writer.finish().unwrap()).unwrap();
        assert_eq!(Some("/foo/example.txt"), archive.name_for_index(0));
    }

    #[test]
    fn write_symlink_wonky_paths() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .add_symlink(
                "directory\\link",
                "/absolute/symlink\\with\\mixed/slashes",
                SimpleFileOptions::default().last_modified_time(
                    DateTime::from_date_and_time(2018, 8, 15, 20, 45, 6).unwrap(),
                ),
            )
            .unwrap();
        assert!(writer
            .write(b"writing to a symlink is not allowed and will not write any data")
            .is_err());
        let result = writer.finish().unwrap();
        assert_eq!(result.get_ref().len(), 162);
        assert_eq!(
            *result.get_ref(),
            &[
                80u8, 75, 3, 4, 20, 0, 0, 0, 0, 0, 163, 165, 15, 77, 95, 41, 81, 245, 36, 0, 0, 0,
                36, 0, 0, 0, 14, 0, 0, 0, 100, 105, 114, 101, 99, 116, 111, 114, 121, 92, 108, 105,
                110, 107, 47, 97, 98, 115, 111, 108, 117, 116, 101, 47, 115, 121, 109, 108, 105,
                110, 107, 92, 119, 105, 116, 104, 92, 109, 105, 120, 101, 100, 47, 115, 108, 97,
                115, 104, 101, 115, 80, 75, 1, 2, 46, 3, 20, 0, 0, 0, 0, 0, 163, 165, 15, 77, 95,
                41, 81, 245, 36, 0, 0, 0, 36, 0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255,
                161, 0, 0, 0, 0, 100, 105, 114, 101, 99, 116, 111, 114, 121, 92, 108, 105, 110,
                107, 80, 75, 5, 6, 0, 0, 0, 0, 1, 0, 1, 0, 60, 0, 0, 0, 80, 0, 0, 0, 0, 0
            ] as &[u8],
        );
    }

    #[test]
    fn write_mimetype_zip() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        let options = FileOptions {
            compression_method: CompressionMethod::Stored,
            compression_level: None,
            last_modified_time: DateTime::default(),
            permissions: Some(33188),
            large_file: false,
            encrypt_with: None,
            extended_options: (),
            alignment: 1,
            #[cfg(feature = "deflate-zopfli")]
            zopfli_buffer_size: None,
        };
        writer.start_file("mimetype", options).unwrap();
        writer
            .write_all(b"application/vnd.oasis.opendocument.text")
            .unwrap();
        let result = writer.finish().unwrap();

        assert_eq!(result.get_ref().len(), 153);
        let mut v = Vec::new();
        v.extend_from_slice(include_bytes!("../tests/data/mimetype.zip"));
        assert_eq!(result.get_ref(), &v);
    }

    const RT_TEST_TEXT: &str = "And I can't stop thinking about the moments that I lost to you\
                            And I can't stop thinking of things I used to do\
                            And I can't stop making bad decisions\
                            And I can't stop eating stuff you make me chew\
                            I put on a smile like you wanna see\
                            Another day goes by that I long to be like you";
    const RT_TEST_FILENAME: &str = "subfolder/sub-subfolder/can't_stop.txt";
    const SECOND_FILENAME: &str = "different_name.xyz";
    const THIRD_FILENAME: &str = "third_name.xyz";

    #[test]
    fn test_shallow_copy() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        let options = FileOptions {
            compression_method: CompressionMethod::default(),
            compression_level: None,
            last_modified_time: DateTime::default(),
            permissions: Some(33188),
            large_file: false,
            encrypt_with: None,
            extended_options: (),
            alignment: 0,
            #[cfg(feature = "deflate-zopfli")]
            zopfli_buffer_size: None,
        };
        writer.start_file(RT_TEST_FILENAME, options).unwrap();
        writer.write_all(RT_TEST_TEXT.as_ref()).unwrap();
        writer
            .shallow_copy_file(RT_TEST_FILENAME, SECOND_FILENAME)
            .unwrap();
        writer
            .shallow_copy_file(RT_TEST_FILENAME, SECOND_FILENAME)
            .expect_err("Duplicate filename");
        let zip = writer.finish().unwrap();
        let mut writer = ZipWriter::new_append(zip).unwrap();
        writer
            .shallow_copy_file(SECOND_FILENAME, SECOND_FILENAME)
            .expect_err("Duplicate filename");
        let zip = writer.finish().unwrap();
        let mut reader = ZipArchive::new(zip).unwrap();
        let mut file_names: Vec<&str> = reader.file_names().collect();
        file_names.sort();
        let mut expected_file_names = vec![RT_TEST_FILENAME, SECOND_FILENAME];
        expected_file_names.sort();
        assert_eq!(file_names, expected_file_names);
        let mut first_file_content = String::new();
        reader
            .by_name(RT_TEST_FILENAME)
            .unwrap()
            .read_to_string(&mut first_file_content)
            .unwrap();
        assert_eq!(first_file_content, RT_TEST_TEXT);
        let mut second_file_content = String::new();
        reader
            .by_name(SECOND_FILENAME)
            .unwrap()
            .read_to_string(&mut second_file_content)
            .unwrap();
        assert_eq!(second_file_content, RT_TEST_TEXT);
    }

    #[test]
    fn test_deep_copy() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        let options = FileOptions {
            compression_method: CompressionMethod::default(),
            compression_level: None,
            last_modified_time: DateTime::default(),
            permissions: Some(33188),
            large_file: false,
            encrypt_with: None,
            extended_options: (),
            alignment: 0,
            #[cfg(feature = "deflate-zopfli")]
            zopfli_buffer_size: None,
        };
        writer.start_file(RT_TEST_FILENAME, options).unwrap();
        writer.write_all(RT_TEST_TEXT.as_ref()).unwrap();
        writer
            .deep_copy_file(RT_TEST_FILENAME, SECOND_FILENAME)
            .unwrap();
        let zip = writer.finish().unwrap();
        let mut writer = ZipWriter::new_append(zip).unwrap();
        writer
            .deep_copy_file(RT_TEST_FILENAME, THIRD_FILENAME)
            .unwrap();
        let zip = writer.finish().unwrap();
        let mut reader = ZipArchive::new(zip).unwrap();
        let mut file_names: Vec<&str> = reader.file_names().collect();
        file_names.sort();
        let mut expected_file_names = vec![RT_TEST_FILENAME, SECOND_FILENAME, THIRD_FILENAME];
        expected_file_names.sort();
        assert_eq!(file_names, expected_file_names);
        let mut first_file_content = String::new();
        reader
            .by_name(RT_TEST_FILENAME)
            .unwrap()
            .read_to_string(&mut first_file_content)
            .unwrap();
        assert_eq!(first_file_content, RT_TEST_TEXT);
        let mut second_file_content = String::new();
        reader
            .by_name(SECOND_FILENAME)
            .unwrap()
            .read_to_string(&mut second_file_content)
            .unwrap();
        assert_eq!(second_file_content, RT_TEST_TEXT);
    }

    #[test]
    fn duplicate_filenames() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file("foo/bar/test", SimpleFileOptions::default())
            .unwrap();
        writer
            .write_all("The quick brown 🦊 jumps over the lazy 🐕".as_bytes())
            .unwrap();
        writer
            .start_file("foo/bar/test", SimpleFileOptions::default())
            .expect_err("Expected duplicate filename not to be allowed");
    }

    #[test]
    fn test_filename_looks_like_zip64_locator() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file(
                "PK\u{6}\u{7}\0\0\0\u{11}\0\0\0\0\0\0\0\0\0\0\0\0",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        let _ = ZipArchive::new(zip).unwrap();
    }

    #[test]
    fn test_filename_looks_like_zip64_locator_2() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file(
                "PK\u{6}\u{6}\0\0\0\0\0\0\0\0\0\0PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        println!("{:02x?}", zip.get_ref());
        let _ = ZipArchive::new(zip).unwrap();
    }

    #[test]
    fn test_filename_looks_like_zip64_locator_2a() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file(
                "PK\u{6}\u{6}PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        println!("{:02x?}", zip.get_ref());
        let _ = ZipArchive::new(zip).unwrap();
    }

    #[test]
    fn test_filename_looks_like_zip64_locator_3() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file("\0PK\u{6}\u{6}", SimpleFileOptions::default())
            .unwrap();
        writer
            .start_file(
                "\0\u{4}\0\0PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\u{3}",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        println!("{:02x?}", zip.get_ref());
        let _ = ZipArchive::new(zip).unwrap();
    }

    #[test]
    fn test_filename_looks_like_zip64_locator_4() {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file("PK\u{6}\u{6}", SimpleFileOptions::default())
            .unwrap();
        writer
            .start_file("\0\0\0\0\0\0", SimpleFileOptions::default())
            .unwrap();
        writer
            .start_file("\0", SimpleFileOptions::default())
            .unwrap();
        writer.start_file("", SimpleFileOptions::default()).unwrap();
        writer
            .start_file("\0\0", SimpleFileOptions::default())
            .unwrap();
        writer
            .start_file(
                "\0\0\0PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        println!("{:02x?}", zip.get_ref());
        let _ = ZipArchive::new(zip).unwrap();
    }

    #[test]
    fn test_filename_looks_like_zip64_locator_5() -> ZipResult<()> {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .add_directory("", SimpleFileOptions::default().with_alignment(21))
            .unwrap();
        let mut writer = ZipWriter::new_append(writer.finish().unwrap()).unwrap();
        writer.shallow_copy_file("/", "").unwrap();
        writer.shallow_copy_file("", "\0").unwrap();
        writer.shallow_copy_file("\0", "PK\u{6}\u{6}").unwrap();
        let mut writer = ZipWriter::new_append(writer.finish().unwrap()).unwrap();
        writer
            .start_file("\0\0\0\0\0\0", SimpleFileOptions::default())
            .unwrap();
        let mut writer = ZipWriter::new_append(writer.finish().unwrap()).unwrap();
        writer
            .start_file(
                "#PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
                SimpleFileOptions::default(),
            )
            .unwrap();
        let zip = writer.finish().unwrap();
        println!("{:02x?}", zip.get_ref());
        let _ = ZipArchive::new(zip).unwrap();
        Ok(())
    }

    #[test]
    fn remove_shallow_copy_keeps_original() -> ZipResult<()> {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer
            .start_file("original", SimpleFileOptions::default())
            .unwrap();
        writer.write_all(RT_TEST_TEXT.as_bytes()).unwrap();
        writer
            .shallow_copy_file("original", "shallow_copy")
            .unwrap();
        writer.abort_file().unwrap();
        let mut zip = ZipArchive::new(writer.finish().unwrap()).unwrap();
        let mut file = zip.by_name("original").unwrap();
        let mut contents = Vec::new();
        file.read_to_end(&mut contents).unwrap();
        assert_eq!(RT_TEST_TEXT.as_bytes(), contents);
        Ok(())
    }

    #[test]
    fn remove_encrypted_file() -> ZipResult<()> {
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        let first_file_options = SimpleFileOptions::default()
            .with_alignment(65535)
            .with_deprecated_encryption(b"Password");
        writer.start_file("", first_file_options).unwrap();
        writer.abort_file().unwrap();
        let zip = writer.finish().unwrap();
        let mut writer = ZipWriter::new(zip);
        writer.start_file("", SimpleFileOptions::default()).unwrap();
        Ok(())
    }

    #[test]
    fn remove_encrypted_aligned_symlink() -> ZipResult<()> {
        let mut options = SimpleFileOptions::default();
        options = options.with_deprecated_encryption(b"Password");
        options.alignment = 65535;
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer.add_symlink("", "s\t\0\0ggggg\0\0", options).unwrap();
        writer.abort_file().unwrap();
        let zip = writer.finish().unwrap();
        println!("{:0>2x?}", zip.get_ref());
        let mut writer = ZipWriter::new_append(zip).unwrap();
        writer.start_file("", SimpleFileOptions::default()).unwrap();
        Ok(())
    }

    #[cfg(feature = "deflate-zopfli")]
    #[test]
    fn zopfli_empty_write() -> ZipResult<()> {
        let mut options = SimpleFileOptions::default();
        options = options
            .compression_method(CompressionMethod::default())
            .compression_level(Some(264));
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        writer.start_file("", options).unwrap();
        writer.write_all(&[]).unwrap();
        writer.write_all(&[]).unwrap();
        Ok(())
    }

    #[test]
    fn crash_with_no_features() -> ZipResult<()> {
        const ORIGINAL_FILE_NAME: &str = "PK\u{6}\u{6}\0\0\0\0\0\0\0\0\0\u{2}g\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\u{1}\0\0\0\0\0\0\0\0\0\0PK\u{6}\u{7}\0\0\0\0\0\0\0\0\0\0\0\0\u{7}\0\t'";
        let mut writer = ZipWriter::new(io::Cursor::new(Vec::new()));
        let mut options = SimpleFileOptions::default();
        options = options
            .with_alignment(3584)
            .compression_method(CompressionMethod::Stored);
        writer.start_file(ORIGINAL_FILE_NAME, options)?;
        let archive = writer.finish()?;
        let mut writer = ZipWriter::new_append(archive)?;
        writer.shallow_copy_file(ORIGINAL_FILE_NAME, "\u{6}\\")?;
        writer.finish()?;
        Ok(())
    }

    #[test]
    fn test_alignment() {
        let page_size = 4096;
        let options = SimpleFileOptions::default()
            .compression_method(CompressionMethod::Stored)
            .with_alignment(page_size);
        let mut zip = ZipWriter::new(io::Cursor::new(Vec::new()));
        let contents = b"sleeping";
        let () = zip.start_file("sleep", options).unwrap();
        let _count = zip.write(&contents[..]).unwrap();
        let mut zip = ZipArchive::new(zip.finish().unwrap()).unwrap();
        let file = zip.by_index(0).unwrap();
        assert_eq!(file.name(), "sleep");
        assert_eq!(file.data_start(), page_size.into());
    }

    #[test]
    fn test_crash_short_read() {
        let mut writer = ZipWriter::new(Cursor::new(Vec::new()));
        let comment: Vec<u8> = vec![
            1, 80, 75, 5, 6, 237, 237, 237, 237, 237, 237, 237, 237, 44, 255, 191, 255, 255, 255,
            255, 255, 255, 255, 255, 16,
        ];
        writer.set_raw_comment(comment);
        let options = SimpleFileOptions::default()
            .compression_method(Stored)
            .with_alignment(11823);
        writer.start_file("", options).unwrap();
        writer.write_all(&[255, 255, 44, 255, 0]).unwrap();
        let written = writer.finish().unwrap();
        let _ = ZipWriter::new_append(written).unwrap();
    }
}
