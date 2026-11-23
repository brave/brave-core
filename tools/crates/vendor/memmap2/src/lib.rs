#![deny(clippy::all, clippy::pedantic)]
#![deny(unsafe_op_in_unsafe_fn)]
#![allow(
    // pedantic exceptions
    clippy::cast_possible_truncation,
    clippy::cast_possible_wrap,
    clippy::cast_sign_loss,
    clippy::doc_markdown,
    clippy::explicit_deref_methods,
    clippy::missing_errors_doc,
    clippy::module_name_repetitions,
    clippy::must_use_candidate,
    clippy::needless_pass_by_value,
    clippy::return_self_not_must_use,
    clippy::unreadable_literal,
    clippy::upper_case_acronyms,
)]

//! A cross-platform Rust API for memory mapped buffers.
//!
//! The core functionality is provided by either [`Mmap`] or [`MmapMut`],
//! which correspond to mapping a [`File`] to a [`&[u8]`](https://doc.rust-lang.org/std/primitive.slice.html)
//! or [`&mut [u8]`](https://doc.rust-lang.org/std/primitive.slice.html)
//! respectively. Both function by dereferencing to a slice, allowing the
//! [`Mmap`]/[`MmapMut`] to be used in the same way you would the equivalent slice
//! types.
//!
//! [`File`]: std::fs::File
//!
//! # Examples
//!
//! For simple cases [`Mmap`] can be used directly:
//!
//! ```
//! use std::fs::File;
//! use std::io::Read;
//!
//! use memmap2::Mmap;
//!
//! # fn main() -> std::io::Result<()> {
//! let mut file = File::open("LICENSE-APACHE")?;
//!
//! let mut contents = Vec::new();
//! file.read_to_end(&mut contents)?;
//!
//! let mmap = unsafe { Mmap::map(&file)?  };
//!
//! assert_eq!(&contents[..], &mmap[..]);
//! # Ok(())
//! # }
//! ```
//!
//! However for cases which require configuration of the mapping, then
//! you can use [`MmapOptions`] in order to further configure a mapping
//! before you create it.

#![allow(clippy::len_without_is_empty, clippy::missing_safety_doc)]

#[cfg_attr(unix, path = "unix.rs")]
#[cfg_attr(windows, path = "windows.rs")]
#[cfg_attr(not(any(unix, windows)), path = "stub.rs")]
mod os;
use crate::os::{file_len, MmapInner};

#[cfg(unix)]
mod advice;
#[cfg(unix)]
pub use crate::advice::{Advice, UncheckedAdvice};

use std::fmt;
#[cfg(not(any(unix, windows)))]
use std::fs::File;
use std::io::{Error, ErrorKind};
use std::ops::{Deref, DerefMut};
#[cfg(unix)]
use std::os::unix::io::{AsRawFd, RawFd};
#[cfg(windows)]
use std::os::windows::io::{AsRawHandle, RawHandle};
use std::slice;

#[cfg(not(any(unix, windows)))]
pub struct MmapRawDescriptor<'a>(&'a File);

#[cfg(unix)]
pub struct MmapRawDescriptor(RawFd);

#[cfg(windows)]
pub struct MmapRawDescriptor(RawHandle);

pub trait MmapAsRawDesc {
    fn as_raw_desc(&self) -> MmapRawDescriptor;
}

#[cfg(not(any(unix, windows)))]
impl MmapAsRawDesc for &File {
    fn as_raw_desc(&self) -> MmapRawDescriptor {
        MmapRawDescriptor(self)
    }
}

#[cfg(unix)]
impl MmapAsRawDesc for RawFd {
    fn as_raw_desc(&self) -> MmapRawDescriptor {
        MmapRawDescriptor(*self)
    }
}

#[cfg(unix)]
impl<T> MmapAsRawDesc for &T
where
    T: AsRawFd,
{
    fn as_raw_desc(&self) -> MmapRawDescriptor {
        MmapRawDescriptor(self.as_raw_fd())
    }
}

#[cfg(windows)]
impl MmapAsRawDesc for RawHandle {
    fn as_raw_desc(&self) -> MmapRawDescriptor {
        MmapRawDescriptor(*self)
    }
}

#[cfg(windows)]
impl<T> MmapAsRawDesc for &T
where
    T: AsRawHandle,
{
    fn as_raw_desc(&self) -> MmapRawDescriptor {
        MmapRawDescriptor(self.as_raw_handle())
    }
}

/// Error returned by [`MmapOptions::map_if_safe()`].
///
/// Allows you to distinguish between three failure modes:
///
/// * Safe mapping is not supported on this platform.
/// * Trying to map an unsafe file descriptor.
/// * The map attempt itself failed.
///
/// The error is convertible to [`std::io::Error`].
pub struct MapIfSafeError {
    inner: MapIfSafeErrorInner,
}

#[derive(Debug)]
#[allow(dead_code)] // Not all variants are constructed on all platforms.
enum MapIfSafeErrorInner {
    NotSupported,
    NotSafe(std::io::Error),
    MapFailed(std::io::Error),
}

impl MapIfSafeError {
    #[allow(dead_code)] // Not all variants are constructed on all platforms.
    fn new_not_supported() -> Self {
        let inner = MapIfSafeErrorInner::NotSupported;
        Self { inner }
    }

    #[allow(dead_code)] // Not all variants are constructed on all platforms.
    fn new_not_safe(reason: std::io::Error) -> Self {
        let inner = MapIfSafeErrorInner::NotSafe(reason);
        Self { inner }
    }

    #[allow(dead_code)] // Not all variants are constructed on all platforms.
    fn new_map_failed(reason: std::io::Error) -> Self {
        let inner = MapIfSafeErrorInner::MapFailed(reason);
        Self { inner }
    }

    /// Indicates that safe mapping is not supported at all on this platform.
    pub fn not_supported(&self) -> bool {
        matches!(self.inner, MapIfSafeErrorInner::NotSupported)
    }

    /// Indicates if the map was refused because mapping the file descriptor is not safe.
    pub fn not_safe(&self) -> bool {
        matches!(self.inner, MapIfSafeErrorInner::NotSafe(_))
    }

    /// Indicates if the file descriptor to be mapped was considered safe, but the mapping itself failed.
    pub fn map_failed(&self) -> bool {
        matches!(self.inner, MapIfSafeErrorInner::MapFailed(_))
    }
}

impl std::error::Error for MapIfSafeError {}

impl fmt::Display for MapIfSafeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        #[allow(clippy::match_same_arms)]
        match &self.inner {
            MapIfSafeErrorInner::NotSupported => {
                fmt::Display::fmt(&Error::from(ErrorKind::Unsupported), f)
            }
            MapIfSafeErrorInner::NotSafe(e) => fmt::Display::fmt(e, f),
            MapIfSafeErrorInner::MapFailed(e) => fmt::Display::fmt(e, f),
        }
    }
}

impl fmt::Debug for MapIfSafeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(&self.inner, f)
    }
}

impl From<MapIfSafeError> for std::io::Error {
    fn from(value: MapIfSafeError) -> Self {
        #[allow(clippy::match_same_arms)]
        match value.inner {
            MapIfSafeErrorInner::NotSupported => std::io::ErrorKind::Unsupported.into(),
            MapIfSafeErrorInner::NotSafe(e) => e,
            MapIfSafeErrorInner::MapFailed(e) => e,
        }
    }
}

/// A memory map builder, providing advanced options and flags for specifying memory map behavior.
///
/// `MmapOptions` can be used to create an anonymous memory map using [`map_anon()`], or a
/// file-backed memory map using one of [`map()`], [`map_mut()`], [`map_exec()`],
/// [`map_copy()`], or [`map_copy_read_only()`].
///
/// ## Safety
///
/// All file-backed memory map constructors are marked `unsafe` because of the potential for
/// *Undefined Behavior* (UB) using the map if the underlying file is subsequently modified, in or
/// out of process. Applications must consider the risk and take appropriate precautions when
/// using file-backed maps. Solutions such as file permissions, locks or process-private (e.g.
/// unlinked) files exist but are platform specific and limited.
///
/// [`map_anon()`]: MmapOptions::map_anon()
/// [`map()`]: MmapOptions::map()
/// [`map_mut()`]: MmapOptions::map_mut()
/// [`map_exec()`]: MmapOptions::map_exec()
/// [`map_copy()`]: MmapOptions::map_copy()
/// [`map_copy_read_only()`]: MmapOptions::map_copy_read_only()
#[derive(Clone, Debug, Default)]
#[allow(clippy::struct_excessive_bools)]
pub struct MmapOptions {
    offset: u64,
    len: Option<usize>,
    huge: Option<u8>,
    stack: bool,
    populate: bool,
    no_reserve_swap: bool,
    no_probe_handle: bool,
}

impl MmapOptions {
    /// Creates a new set of options for configuring and creating a memory map.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::{MmapMut, MmapOptions};
    /// # use std::io::Result;
    ///
    /// # fn main() -> Result<()> {
    /// // Create a new memory map builder.
    /// let mut mmap_options = MmapOptions::new();
    ///
    /// // Configure the memory map builder using option setters, then create
    /// // a memory map using one of `mmap_options.map_anon`, `mmap_options.map`,
    /// // `mmap_options.map_mut`, `mmap_options.map_exec`, or `mmap_options.map_copy`:
    /// let mut mmap: MmapMut = mmap_options.len(36).map_anon()?;
    ///
    /// // Use the memory map:
    /// mmap.copy_from_slice(b"...data to copy to the memory map...");
    /// # Ok(())
    /// # }
    /// ```
    pub fn new() -> MmapOptions {
        MmapOptions::default()
    }

    /// Configures the memory map to start at byte `offset` from the beginning of the file.
    ///
    /// This option has no effect on anonymous memory maps.
    ///
    /// By default, the offset is 0.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mmap = unsafe {
    ///     MmapOptions::new()
    ///                 .offset(30)
    ///                 .map(&File::open("LICENSE-APACHE")?)?
    /// };
    /// assert_eq!(&b"Apache License"[..],
    ///            &mmap[..14]);
    /// # Ok(())
    /// # }
    /// ```
    pub fn offset(&mut self, offset: u64) -> &mut Self {
        self.offset = offset;
        self
    }

    /// Configures the created memory mapped buffer to be `len` bytes long.
    ///
    /// This option is mandatory for anonymous memory maps.
    ///
    /// For file-backed memory maps, the length will default to the file length.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mmap = unsafe {
    ///     MmapOptions::new()
    ///                 .len(9)
    ///                 .map(&File::open("README.md")?)?
    /// };
    /// assert_eq!(&b"# memmap2"[..], &mmap[..]);
    /// # Ok(())
    /// # }
    /// ```
    pub fn len(&mut self, len: usize) -> &mut Self {
        self.len = Some(len);
        self
    }

    fn validate_len(len: u64) -> std::io::Result<usize> {
        // Rust's slice cannot be larger than isize::MAX.
        // See https://doc.rust-lang.org/std/slice/fn.from_raw_parts.html
        //
        // This is not a problem on 64-bit targets, but on 32-bit one
        // having a file or an anonymous mapping larger than 2GB is quite normal
        // and we have to prevent it.
        if isize::try_from(len).is_err() {
            return Err(Error::new(
                ErrorKind::InvalidData,
                "memory map length overflows isize",
            ));
        }
        // If an unsigned number (u64) fits in isize, then it fits in usize.
        Ok(len as usize)
    }

    /// Returns the configured length, or the length of the provided file.
    fn get_len<T: MmapAsRawDesc>(&self, file: &T) -> std::io::Result<usize> {
        let len = if let Some(len) = self.len {
            len as u64
        } else {
            let desc = file.as_raw_desc();
            let file_len = file_len(desc.0)?;

            if file_len < self.offset {
                return Err(Error::new(
                    ErrorKind::InvalidData,
                    "memory map offset is larger than length",
                ));
            }

            file_len - self.offset
        };
        Self::validate_len(len)
    }

    /// Configures the anonymous memory map to be suitable for a process or thread stack.
    ///
    /// This option corresponds to the `MAP_STACK` flag on Linux. It has no effect on Windows.
    ///
    /// This option has no effect on file-backed memory maps.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let stack = MmapOptions::new().stack().len(4096).map_anon();
    /// # Ok(())
    /// # }
    /// ```
    pub fn stack(&mut self) -> &mut Self {
        self.stack = true;
        self
    }

    /// Configures the anonymous memory map to be allocated using huge pages.
    ///
    /// This option corresponds to the `MAP_HUGETLB` flag on Linux. It has no effect on Windows.
    ///
    /// The size of the requested page can be specified in page bits. If not provided, the system
    /// default is requested. The requested length should be a multiple of this, or the mapping
    /// will fail.
    ///
    /// This option has no effect on file-backed memory maps.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let stack = MmapOptions::new().huge(Some(21)).len(2*1024*1024).map_anon();
    /// # Ok(())
    /// # }
    /// ```
    ///
    /// The number 21 corresponds to `MAP_HUGE_2MB`. See mmap(2) for more details.
    pub fn huge(&mut self, page_bits: Option<u8>) -> &mut Self {
        self.huge = Some(page_bits.unwrap_or(0));
        self
    }

    /// Populate (prefault) page tables for a mapping.
    ///
    /// For a file mapping, this causes read-ahead on the file. This will help to reduce blocking on page faults later.
    ///
    /// This option corresponds to the `MAP_POPULATE` flag on Linux. It has no effect on Windows.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let file = File::open("LICENSE-MIT")?;
    ///
    /// let mmap = unsafe {
    ///     MmapOptions::new().populate().map(&file)?
    /// };
    ///
    /// assert_eq!(&b"Copyright"[..], &mmap[..9]);
    /// # Ok(())
    /// # }
    /// ```
    pub fn populate(&mut self) -> &mut Self {
        self.populate = true;
        self
    }

    /// Do not reserve swap space for the memory map.
    ///
    /// By default, platforms may reserve swap space for memory maps.
    /// This guarantees that a write to the mapped memory will succeed, even if physical memory is exhausted.
    /// Otherwise, the write to memory could fail (on Linux with a segfault).
    ///
    /// This option requests that no swap space will be allocated for the memory map,
    /// which can be useful for extremely large maps that are only written to sparsely.
    ///
    /// This option is currently supported on Linux, Android, Apple platforms (macOS, iOS, visionOS, etc.), NetBSD, Solaris and Illumos.
    /// On those platforms, this option corresponds to the `MAP_NORESERVE` flag.
    /// On Linux, this option is ignored if [`vm.overcommit_memory`](https://www.kernel.org/doc/Documentation/vm/overcommit-accounting) is set to 2.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let file = File::open("LICENSE-MIT")?;
    ///
    /// let mmap = unsafe {
    ///     MmapOptions::new().no_reserve_swap().map_copy(&file)?
    /// };
    ///
    /// assert_eq!(&b"Copyright"[..], &mmap[..9]);
    /// # Ok(())
    /// # }
    /// ```
    pub fn no_reserve_swap(&mut self) -> &mut Self {
        self.no_reserve_swap = true;
        self
    }

    /// Do not probe the file handle for the widest supported protection
    /// level when creating the mapping.
    ///
    /// This option only has an effect on Windows. There, by default,
    /// creating a mapping issues up to two extra `CreateFileMappingW` calls
    /// to discover whether the handle supports write and/or execute access,
    /// so that the underlying section is created with the widest protection
    /// the handle supports and the mapping can later be transitioned, e.g.
    /// with [`Mmap::make_mut()`].
    ///
    /// With this option set, the section and view are created with exactly
    /// the protection the mapping type requires (e.g. `PAGE_READONLY` and
    /// `FILE_MAP_READ` for [`map()`][MmapOptions::map]). This guarantees a
    /// read-only mapping never creates a writable section on the file, but
    /// transitioning the returned mapping to a wider protection will fail.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let file = File::open("LICENSE-MIT")?;
    ///
    /// let mmap = unsafe {
    ///     MmapOptions::new().no_probe_handle().map(&file)?
    /// };
    ///
    /// assert_eq!(&b"Copyright"[..], &mmap[..9]);
    /// # Ok(())
    /// # }
    /// ```
    pub fn no_probe_handle(&mut self) -> &mut Self {
        self.no_probe_handle = true;
        self
    }

    /// Creates a read-only memory map backed by a file.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapOptions] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    /// use std::io::Read;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mut file = File::open("LICENSE-APACHE")?;
    ///
    /// let mut contents = Vec::new();
    /// file.read_to_end(&mut contents)?;
    ///
    /// let mmap = unsafe {
    ///     MmapOptions::new().map(&file)?
    /// };
    ///
    /// assert_eq!(&contents[..], &mmap[..]);
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<Mmap> {
        let desc = file.as_raw_desc();

        MmapInner::map(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| Mmap { inner })
    }

    /// Creates a readable and executable memory map backed by a file.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapOptions] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub unsafe fn map_exec<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<Mmap> {
        let desc = file.as_raw_desc();

        MmapInner::map_exec(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| Mmap { inner })
    }

    /// Creates a writeable memory map backed by a file.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapOptions] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read and write permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use std::fs::OpenOptions;
    /// use std::path::PathBuf;
    ///
    /// use memmap2::MmapOptions;
    /// #
    /// # fn main() -> std::io::Result<()> {
    /// # let tempdir = tempfile::tempdir()?;
    /// let path: PathBuf = /* path to file */
    /// #   tempdir.path().join("map_mut");
    /// let file = OpenOptions::new().read(true).write(true).create(true).truncate(true).open(&path)?;
    /// file.set_len(13)?;
    ///
    /// let mut mmap = unsafe {
    ///     MmapOptions::new().map_mut(&file)?
    /// };
    ///
    /// mmap.copy_from_slice(b"Hello, world!");
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map_mut<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<MmapMut> {
        let desc = file.as_raw_desc();

        MmapInner::map_mut(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| MmapMut { inner })
    }

    /// Creates a copy-on-write memory map backed by a file.
    ///
    /// Data written to the memory map will not be visible by other processes,
    /// and will not be carried through to the underlying file.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapOptions] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with writable permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    /// use std::io::Write;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let file = File::open("LICENSE-APACHE")?;
    /// let mut mmap = unsafe { MmapOptions::new().map_copy(&file)? };
    /// (&mut mmap[..]).write_all(b"Hello, world!")?;
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map_copy<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<MmapMut> {
        let desc = file.as_raw_desc();

        MmapInner::map_copy(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| MmapMut { inner })
    }

    /// Creates a copy-on-write read-only memory map backed by a file.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapOptions] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::MmapOptions;
    /// use std::fs::File;
    /// use std::io::Read;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mut file = File::open("README.md")?;
    ///
    /// let mut contents = Vec::new();
    /// file.read_to_end(&mut contents)?;
    ///
    /// let mmap = unsafe {
    ///     MmapOptions::new().map_copy_read_only(&file)?
    /// };
    ///
    /// assert_eq!(&contents[..], &mmap[..]);
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map_copy_read_only<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<Mmap> {
        let desc = file.as_raw_desc();

        MmapInner::map_copy_read_only(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| Mmap { inner })
    }

    /// Creates a read-only memory map backed by a file, after
    /// verifying that the backing file is immutable.
    ///
    /// Unlike [`map()`], this method is not `unsafe` because it verifies that
    /// the underlying file cannot be modified or truncated. The mapped region
    /// must fit within the current file size.
    ///
    /// Returns `Ok(None)` if the file cannot be proven immutable (including
    /// on platforms where this check is not supported). Returns `Err` only
    /// for unexpected I/O errors.
    ///
    /// Currently, immutability is recognized on Linux, Android, and FreeBSD
    /// via:
    ///
    /// - A memfd sealed with at least `F_SEAL_SHRINK` and `F_SEAL_WRITE`, or
    /// - A file with fs-verity enabled (Linux and Android only).
    ///
    /// [`map()`]: MmapOptions::map()
    pub fn map_if_safe<T: MmapAsRawDesc>(&self, file: T) -> Result<Mmap, MapIfSafeError> {
        let desc = file.as_raw_desc();
        let len = self
            .get_len(&file)
            .map_err(MapIfSafeError::new_map_failed)?;
        MmapInner::check_safe_to_map(desc.0, self.offset, len)?;
        MmapInner::map(
            len,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map_err(MapIfSafeError::new_map_failed)
        .map(|inner| Mmap { inner })
    }

    /// Creates an anonymous memory map.
    ///
    /// The memory map length should be configured using [`MmapOptions::len()`]
    /// before creating an anonymous memory map, otherwise a zero-length mapping
    /// will be created.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails or
    /// when `len > isize::MAX`.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub fn map_anon(&self) -> std::io::Result<MmapMut> {
        let len = self.len.unwrap_or(0);

        // See get_len() for details.
        let len = Self::validate_len(len as u64)?;

        MmapInner::map_anon(
            len,
            self.stack,
            self.populate,
            self.huge,
            self.no_reserve_swap,
        )
        .map(|inner| MmapMut { inner })
    }

    /// Creates a raw memory map.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read and write permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub fn map_raw<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<MmapRaw> {
        let desc = file.as_raw_desc();

        MmapInner::map_mut(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| MmapRaw { inner })
    }

    /// Creates a read-only raw memory map
    ///
    /// This is primarily useful to avoid intermediate `Mmap` instances when
    /// read-only access to files modified elsewhere are required.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub fn map_raw_read_only<T: MmapAsRawDesc>(&self, file: T) -> std::io::Result<MmapRaw> {
        let desc = file.as_raw_desc();

        MmapInner::map(
            self.get_len(&file)?,
            desc.0,
            self.offset,
            self.populate,
            self.no_reserve_swap,
            !self.no_probe_handle,
        )
        .map(|inner| MmapRaw { inner })
    }
}

/// A handle to an immutable memory mapped buffer.
///
/// A `Mmap` may be backed by a file, or it can be anonymous map, backed by volatile memory. Use
/// [`MmapOptions`] or [`map()`] to create a file-backed memory map. To create an immutable
/// anonymous memory map, first create a mutable anonymous memory map, and then make it immutable
/// with [`MmapMut::make_read_only()`].
///
/// A file backed `Mmap` is created by `&File` reference, and will remain valid even after the
/// `File` is dropped. In other words, the `Mmap` handle is completely independent of the `File`
/// used to create it. For consistency, on some platforms this is achieved by duplicating the
/// underlying file handle. The memory will be unmapped when the `Mmap` handle is dropped.
///
/// Dereferencing and accessing the bytes of the buffer may result in page faults (e.g. swapping
/// the mapped pages into physical memory) though the details of this are platform specific.
///
/// `Mmap` is [`Sync`] and [`Send`].
///
/// See [`MmapMut`] for the mutable version.
///
/// ## Safety
///
/// All file-backed memory map constructors are marked `unsafe` because of the potential for
/// *Undefined Behavior* (UB) using the map if the underlying file is subsequently modified, in or
/// out of process. Applications must consider the risk and take appropriate precautions when using
/// file-backed maps. Solutions such as file permissions, locks or process-private (e.g. unlinked)
/// files exist but are platform specific and limited.
///
/// ## Example
///
/// ```
/// use memmap2::MmapOptions;
/// use std::io::Write;
/// use std::fs::File;
///
/// # fn main() -> std::io::Result<()> {
/// let file = File::open("README.md")?;
/// let mmap = unsafe { MmapOptions::new().map(&file)? };
/// assert_eq!(b"# memmap2", &mmap[0..9]);
/// # Ok(())
/// # }
/// ```
///
/// [`map()`]: Mmap::map()
pub struct Mmap {
    inner: MmapInner,
}

impl Mmap {
    /// Creates a read-only memory map backed by a file.
    ///
    /// This is equivalent to calling `MmapOptions::new().map(file)`.
    ///
    /// # Safety
    ///
    /// See the [type-level][Mmap] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use std::fs::File;
    /// use std::io::Read;
    ///
    /// use memmap2::Mmap;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mut file = File::open("LICENSE-APACHE")?;
    ///
    /// let mut contents = Vec::new();
    /// file.read_to_end(&mut contents)?;
    ///
    /// let mmap = unsafe { Mmap::map(&file)?  };
    ///
    /// assert_eq!(&contents[..], &mmap[..]);
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map<T: MmapAsRawDesc>(file: T) -> std::io::Result<Mmap> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe { MmapOptions::new().map(file) }
    }

    /// Creates a read-only memory map backed by a file, verifying that the
    /// file is immutable.
    ///
    /// This is equivalent to calling `MmapOptions::new().map_if_safe(file)`.
    /// See [`MmapOptions::map_if_safe()`] for details.
    pub fn map_if_safe<T: MmapAsRawDesc>(file: T) -> Result<Mmap, MapIfSafeError> {
        MmapOptions::new().map_if_safe(file)
    }

    /// Transition the memory map to be writable.
    ///
    /// If the memory map is file-backed, the file must have been opened with write permissions.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with writable permissions.
    ///
    /// # Example
    ///
    /// ```
    /// use memmap2::Mmap;
    /// use std::ops::DerefMut;
    /// use std::io::Write;
    /// # use std::fs::OpenOptions;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// # let tempdir = tempfile::tempdir()?;
    /// let file = /* file opened with write permissions */
    /// #          OpenOptions::new()
    /// #                      .read(true)
    /// #                      .write(true)
    /// #                      .create(true)
    /// #                      .truncate(true)
    /// #                      .open(tempdir.path()
    /// #                      .join("make_mut"))?;
    /// # file.set_len(128)?;
    /// let mmap = unsafe { Mmap::map(&file)? };
    /// // ... use the read-only memory map ...
    /// let mut mut_mmap = mmap.make_mut()?;
    /// mut_mmap.deref_mut().write_all(b"hello, world!")?;
    /// # Ok(())
    /// # }
    /// ```
    pub fn make_mut(mut self) -> std::io::Result<MmapMut> {
        self.inner.make_mut()?;
        Ok(MmapMut { inner: self.inner })
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise(&self, advice: Advice) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    ///
    /// # Safety
    /// This function can modify the memory map in ways that do not fit in Rust's safe memory access model.
    /// Care must be taken not to break the soundness rules of the Rust compiler.
    /// Refer to the operating system documentation to see what each of the [`UncheckedAdvice`] variant does.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise(&self, advice: UncheckedAdvice) -> std::io::Result<()> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// Only supported on Unix.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise_range(&self, advice: Advice, offset: usize, len: usize) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    ///
    /// # Safety
    /// This function can modify the memory map in ways that do not fit in Rust's safe memory access model.
    /// Care must be taken not to break the soundness rules of the Rust compiler.
    /// Refer to the operating system documentation to see what each of the [`UncheckedAdvice`] variant does.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise_range(
        &self,
        advice: UncheckedAdvice,
        offset: usize,
        len: usize,
    ) -> std::io::Result<()> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Lock the whole memory map into RAM. Only supported on Unix.
    ///
    /// See [mlock()](https://man7.org/linux/man-pages/man2/mlock.2.html) map page.
    #[cfg(unix)]
    pub fn lock(&self) -> std::io::Result<()> {
        self.inner.lock()
    }

    /// Unlock the whole memory map. Only supported on Unix.
    ///
    /// See [munlock()](https://man7.org/linux/man-pages/man2/munlock.2.html) map page.
    #[cfg(unix)]
    pub fn unlock(&self) -> std::io::Result<()> {
        self.inner.unlock()
    }

    /// Adjust the size of the memory mapping.
    ///
    /// This will try to resize the memory mapping in place. If
    /// [`RemapOptions::may_move`] is specified it will move the mapping if it
    /// could not resize in place, otherwise it will error.
    ///
    /// Only supported on Linux.
    ///
    /// See the [`mremap(2)`] man page.
    ///
    /// # Safety
    ///
    /// Resizing the memory mapping beyond the end of the mapped file will
    /// result in UB should you happen to access memory beyond the end of the
    /// file.
    ///
    /// [`mremap(2)`]: https://man7.org/linux/man-pages/man2/mremap.2.html
    #[cfg(target_os = "linux")]
    pub unsafe fn remap(&mut self, new_len: usize, options: RemapOptions) -> std::io::Result<()> {
        self.inner.remap(new_len, options)
    }
}

#[cfg(feature = "stable_deref_trait")]
unsafe impl stable_deref_trait::StableDeref for Mmap {}

impl Deref for Mmap {
    type Target = [u8];

    #[inline]
    fn deref(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.inner.ptr(), self.inner.len()) }
    }
}

impl AsRef<[u8]> for Mmap {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.deref()
    }
}

impl fmt::Debug for Mmap {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        fmt.debug_struct("Mmap")
            .field("ptr", &self.as_ptr())
            .field("len", &self.len())
            .finish()
    }
}

/// A handle to a raw memory mapped buffer.
///
/// This struct never hands out references to its interior, only raw pointers.
/// This can be helpful when creating shared memory maps between untrusted processes.
///
/// For the safety concerns that arise when converting these raw pointers to references,
/// see the [`Mmap`] safety documentation.
pub struct MmapRaw {
    inner: MmapInner,
}

impl MmapRaw {
    /// Creates a writeable memory map backed by a file.
    ///
    /// This is equivalent to calling `MmapOptions::new().map_raw(file)`.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read and write permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub fn map_raw<T: MmapAsRawDesc>(file: T) -> std::io::Result<MmapRaw> {
        MmapOptions::new().map_raw(file)
    }

    /// Returns a raw pointer to the memory mapped file.
    ///
    /// Before dereferencing this pointer, you have to make sure that the file has not been
    /// truncated since the memory map was created.
    /// Avoiding this will not introduce memory safety issues in Rust terms,
    /// but will cause SIGBUS (or equivalent) signal.
    #[inline]
    pub fn as_ptr(&self) -> *const u8 {
        self.inner.ptr()
    }

    /// Returns an unsafe mutable pointer to the memory mapped file.
    ///
    /// Before dereferencing this pointer, you have to make sure that the file has not been
    /// truncated since the memory map was created.
    /// Avoiding this will not introduce memory safety issues in Rust terms,
    /// but will cause SIGBUS (or equivalent) signal.
    #[inline]
    pub fn as_mut_ptr(&self) -> *mut u8 {
        self.inner.ptr().cast_mut()
    }

    /// Returns the length in bytes of the memory map.
    ///
    /// Note that truncating the file can cause the length to change (and render this value unusable).
    #[inline]
    pub fn len(&self) -> usize {
        self.inner.len()
    }

    /// Flushes outstanding memory map modifications to disk.
    ///
    /// When this method returns with a non-error result, all outstanding changes to a file-backed
    /// memory map are guaranteed to be durably stored. The file's metadata (including last
    /// modification timestamp) may not be updated.
    ///
    /// # Example
    ///
    /// ```
    /// use std::fs::OpenOptions;
    /// use std::io::Write;
    /// use std::path::PathBuf;
    /// use std::slice;
    ///
    /// use memmap2::MmapRaw;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let tempdir = tempfile::tempdir()?;
    /// let path: PathBuf = /* path to file */
    /// #   tempdir.path().join("flush");
    /// let file = OpenOptions::new().read(true).write(true).create(true).truncate(true).open(&path)?;
    /// file.set_len(128)?;
    ///
    /// let mut mmap = unsafe { MmapRaw::map_raw(&file)? };
    ///
    /// let mut memory = unsafe { slice::from_raw_parts_mut(mmap.as_mut_ptr(), 128) };
    /// memory.write_all(b"Hello, world!")?;
    /// mmap.flush()?;
    /// # Ok(())
    /// # }
    /// ```
    pub fn flush(&self) -> std::io::Result<()> {
        let len = self.len();
        self.inner.flush(0, len)
    }

    /// Asynchronously flushes outstanding memory map modifications to disk.
    ///
    /// This method initiates flushing modified pages to durable storage, but it will not wait for
    /// the operation to complete before returning. The file's metadata (including last
    /// modification timestamp) may not be updated.
    pub fn flush_async(&self) -> std::io::Result<()> {
        let len = self.len();
        self.inner.flush_async(0, len)
    }

    /// Flushes outstanding memory map modifications in the range to disk.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// When this method returns with a non-error result, all outstanding changes to a file-backed
    /// memory in the range are guaranteed to be durable stored. The file's metadata (including
    /// last modification timestamp) may not be updated. It is not guaranteed the only the changes
    /// in the specified range are flushed; other outstanding changes to the memory map may be
    /// flushed as well.
    pub fn flush_range(&self, offset: usize, len: usize) -> std::io::Result<()> {
        self.inner.flush(offset, len)
    }

    /// Asynchronously flushes outstanding memory map modifications in the range to disk.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// This method initiates flushing modified pages to durable storage, but it will not wait for
    /// the operation to complete before returning. The file's metadata (including last
    /// modification timestamp) may not be updated. It is not guaranteed that the only changes
    /// flushed are those in the specified range; other outstanding changes to the memory map may
    /// be flushed as well.
    pub fn flush_async_range(&self, offset: usize, len: usize) -> std::io::Result<()> {
        self.inner.flush_async(offset, len)
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise(&self, advice: Advice) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    ///
    /// # Safety
    /// This function can modify the memory map in ways that do not fit in Rust's safe memory access model.
    /// Care must be taken not to break the soundness rules of the Rust compiler.
    /// Refer to the operating system documentation to see what each of the [`UncheckedAdvice`] variant does.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise(&self, advice: UncheckedAdvice) -> std::io::Result<()> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise_range(&self, advice: Advice, offset: usize, len: usize) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    ///
    /// # Safety
    /// This function can modify the memory map in ways that do not fit in Rust's safe memory access model.
    /// Care must be taken not to break the soundness rules of the Rust compiler.
    /// Refer to the operating system documentation to see what each of the [`UncheckedAdvice`] variant does.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise_range(
        &self,
        advice: UncheckedAdvice,
        offset: usize,
        len: usize,
    ) -> std::io::Result<()> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Lock the whole memory map into RAM. Only supported on Unix.
    ///
    /// See [mlock()](https://man7.org/linux/man-pages/man2/mlock.2.html) map page.
    #[cfg(unix)]
    pub fn lock(&self) -> std::io::Result<()> {
        self.inner.lock()
    }

    /// Unlock the whole memory map. Only supported on Unix.
    ///
    /// See [munlock()](https://man7.org/linux/man-pages/man2/munlock.2.html) map page.
    #[cfg(unix)]
    pub fn unlock(&self) -> std::io::Result<()> {
        self.inner.unlock()
    }

    /// Adjust the size of the memory mapping.
    ///
    /// This will try to resize the memory mapping in place. If
    /// [`RemapOptions::may_move`] is specified it will move the mapping if it
    /// could not resize in place, otherwise it will error.
    ///
    /// Only supported on Linux.
    ///
    /// See the [`mremap(2)`] man page.
    ///
    /// # Safety
    ///
    /// Resizing the memory mapping beyond the end of the mapped file will
    /// result in UB should you happen to access memory beyond the end of the
    /// file.
    ///
    /// [`mremap(2)`]: https://man7.org/linux/man-pages/man2/mremap.2.html
    #[cfg(target_os = "linux")]
    pub unsafe fn remap(&mut self, new_len: usize, options: RemapOptions) -> std::io::Result<()> {
        self.inner.remap(new_len, options)
    }
}

impl fmt::Debug for MmapRaw {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        fmt.debug_struct("MmapRaw")
            .field("ptr", &self.as_ptr())
            .field("len", &self.len())
            .finish()
    }
}

impl From<Mmap> for MmapRaw {
    fn from(value: Mmap) -> Self {
        Self { inner: value.inner }
    }
}

impl From<MmapMut> for MmapRaw {
    fn from(value: MmapMut) -> Self {
        Self { inner: value.inner }
    }
}

/// A handle to a mutable memory mapped buffer.
///
/// A file-backed `MmapMut` buffer may be used to read from or write to a file. An anonymous
/// `MmapMut` buffer may be used any place that an in-memory byte buffer is needed. Use
/// [`MmapMut::map_mut()`] and [`MmapMut::map_anon()`] to create a mutable memory map of the
/// respective types, or [`MmapOptions::map_mut()`] and [`MmapOptions::map_anon()`] if non-default
/// options are required.
///
/// A file backed `MmapMut` is created by `&File` reference, and will remain valid even after the
/// `File` is dropped. In other words, the `MmapMut` handle is completely independent of the `File`
/// used to create it. For consistency, on some platforms this is achieved by duplicating the
/// underlying file handle. The memory will be unmapped when the `MmapMut` handle is dropped.
///
/// Dereferencing and accessing the bytes of the buffer may result in page faults (e.g. swapping
/// the mapped pages into physical memory) though the details of this are platform specific.
///
/// `MmapMut` is [`Sync`] and [`Send`].
///
/// See [`Mmap`] for the immutable version.
///
/// ## Safety
///
/// All file-backed memory map constructors are marked `unsafe` because of the potential for
/// *Undefined Behavior* (UB) using the map if the underlying file is subsequently modified, in or
/// out of process. Applications must consider the risk and take appropriate precautions when using
/// file-backed maps. Solutions such as file permissions, locks or process-private (e.g. unlinked)
/// files exist but are platform specific and limited.
pub struct MmapMut {
    inner: MmapInner,
}

impl MmapMut {
    /// Creates a writeable memory map backed by a file.
    ///
    /// This is equivalent to calling `MmapOptions::new().map_mut(file)`.
    ///
    /// # Safety
    ///
    /// See the [type-level][MmapMut] docs for why this function is unsafe.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file is not open with read and write permissions.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    ///
    /// # Example
    ///
    /// ```
    /// use std::fs::OpenOptions;
    /// use std::path::PathBuf;
    ///
    /// use memmap2::MmapMut;
    /// #
    /// # fn main() -> std::io::Result<()> {
    /// # let tempdir = tempfile::tempdir()?;
    /// let path: PathBuf = /* path to file */
    /// #   tempdir.path().join("map_mut");
    /// let file = OpenOptions::new()
    ///                        .read(true)
    ///                        .write(true)
    ///                        .create(true)
    ///                        .truncate(true)
    ///                        .open(&path)?;
    /// file.set_len(13)?;
    ///
    /// let mut mmap = unsafe { MmapMut::map_mut(&file)? };
    ///
    /// mmap.copy_from_slice(b"Hello, world!");
    /// # Ok(())
    /// # }
    /// ```
    pub unsafe fn map_mut<T: MmapAsRawDesc>(file: T) -> std::io::Result<MmapMut> {
        // SAFETY: safety requirements forwarded to caller.
        unsafe { MmapOptions::new().map_mut(file) }
    }

    /// Creates an anonymous memory map.
    ///
    /// This is equivalent to calling `MmapOptions::new().len(length).map_anon()`.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails or
    /// when `len > isize::MAX`.
    ///
    /// Returns [`ErrorKind::Unsupported`] on unsupported platforms.
    pub fn map_anon(length: usize) -> std::io::Result<MmapMut> {
        MmapOptions::new().len(length).map_anon()
    }

    /// Flushes outstanding memory map modifications to disk.
    ///
    /// When this method returns with a non-error result, all outstanding changes to a file-backed
    /// memory map are guaranteed to be durably stored. The file's metadata (including last
    /// modification timestamp) may not be updated.
    ///
    /// # Example
    ///
    /// ```
    /// use std::fs::OpenOptions;
    /// use std::io::Write;
    /// use std::path::PathBuf;
    ///
    /// use memmap2::MmapMut;
    ///
    /// # fn main() -> std::io::Result<()> {
    /// # let tempdir = tempfile::tempdir()?;
    /// let path: PathBuf = /* path to file */
    /// #   tempdir.path().join("flush");
    /// let file = OpenOptions::new().read(true).write(true).create(true).truncate(true).open(&path)?;
    /// file.set_len(128)?;
    ///
    /// let mut mmap = unsafe { MmapMut::map_mut(&file)? };
    ///
    /// (&mut mmap[..]).write_all(b"Hello, world!")?;
    /// mmap.flush()?;
    /// # Ok(())
    /// # }
    /// ```
    pub fn flush(&self) -> std::io::Result<()> {
        let len = self.len();
        self.inner.flush(0, len)
    }

    /// Asynchronously flushes outstanding memory map modifications to disk.
    ///
    /// This method initiates flushing modified pages to durable storage, but it will not wait for
    /// the operation to complete before returning. The file's metadata (including last
    /// modification timestamp) may not be updated.
    pub fn flush_async(&self) -> std::io::Result<()> {
        let len = self.len();
        self.inner.flush_async(0, len)
    }

    /// Flushes outstanding memory map modifications in the range to disk.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// When this method returns with a non-error result, all outstanding changes to a file-backed
    /// memory in the range are guaranteed to be durable stored. The file's metadata (including
    /// last modification timestamp) may not be updated. It is not guaranteed the only the changes
    /// in the specified range are flushed; other outstanding changes to the memory map may be
    /// flushed as well.
    pub fn flush_range(&self, offset: usize, len: usize) -> std::io::Result<()> {
        self.inner.flush(offset, len)
    }

    /// Asynchronously flushes outstanding memory map modifications in the range to disk.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// This method initiates flushing modified pages to durable storage, but it will not wait for
    /// the operation to complete before returning. The file's metadata (including last
    /// modification timestamp) may not be updated. It is not guaranteed that the only changes
    /// flushed are those in the specified range; other outstanding changes to the memory map may
    /// be flushed as well.
    pub fn flush_async_range(&self, offset: usize, len: usize) -> std::io::Result<()> {
        self.inner.flush_async(offset, len)
    }

    /// Returns an immutable version of this memory mapped buffer.
    ///
    /// If the memory map is file-backed, the file must have been opened with read permissions.
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file has not been opened with read permissions.
    ///
    /// # Example
    ///
    /// ```
    /// use std::io::Write;
    /// use std::path::PathBuf;
    ///
    /// use memmap2::{Mmap, MmapMut};
    ///
    /// # fn main() -> std::io::Result<()> {
    /// let mut mmap = MmapMut::map_anon(128)?;
    ///
    /// (&mut mmap[..]).write(b"Hello, world!")?;
    ///
    /// let mmap: Mmap = mmap.make_read_only()?;
    /// # Ok(())
    /// # }
    /// ```
    pub fn make_read_only(mut self) -> std::io::Result<Mmap> {
        self.inner.make_read_only()?;
        Ok(Mmap { inner: self.inner })
    }

    /// Transition the memory map to be readable and executable.
    ///
    /// If the memory map is file-backed, the file must have been opened with execute permissions.
    ///
    /// On systems with separate instructions and data caches (a category that includes many ARM
    /// chips), a platform-specific call may be needed to ensure that the changes are visible to the
    /// execution unit (e.g. when using this function to implement a JIT compiler).  For more
    /// details, see [this ARM write-up](https://community.arm.com/arm-community-blogs/b/architectures-and-processors-blog/posts/caches-and-self-modifying-code)
    /// or the `man` page for [`sys_icache_invalidate`](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/sys_icache_invalidate.3.html).
    ///
    /// # Errors
    ///
    /// This method returns an error when the underlying system call fails, which can happen for a
    /// variety of reasons, such as when the file has not been opened with execute permissions.
    pub fn make_exec(mut self) -> std::io::Result<Mmap> {
        self.inner.make_exec()?;
        Ok(Mmap { inner: self.inner })
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise(&self, advice: Advice) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise(&self, advice: UncheckedAdvice) -> std::io::Result<()> {
        // SAFETY: Safety requirements pushed to caller.
        unsafe {
            self.inner
                .advise(advice as libc::c_int, 0, self.inner.len())
        }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// Only supported on Unix.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub fn advise_range(&self, advice: Advice, offset: usize, len: usize) -> std::io::Result<()> {
        // SAFETY: The `Advice` enum only allows safe advice values.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Advise OS how this range of memory map will be accessed.
    ///
    /// Used with the [unchecked flags][UncheckedAdvice]. Only supported on Unix.
    ///
    /// The offset and length must be in the bounds of the memory map.
    ///
    /// See [madvise()](https://man7.org/linux/man-pages/man2/madvise.2.html) map page.
    #[cfg(unix)]
    pub unsafe fn unchecked_advise_range(
        &self,
        advice: UncheckedAdvice,
        offset: usize,
        len: usize,
    ) -> std::io::Result<()> {
        // SAFETY: Safety requirements pushed to caller.
        unsafe { self.inner.advise(advice as libc::c_int, offset, len) }
    }

    /// Lock the whole memory map into RAM. Only supported on Unix.
    ///
    /// See [mlock()](https://man7.org/linux/man-pages/man2/mlock.2.html) map page.
    #[cfg(unix)]
    pub fn lock(&self) -> std::io::Result<()> {
        self.inner.lock()
    }

    /// Unlock the whole memory map. Only supported on Unix.
    ///
    /// See [munlock()](https://man7.org/linux/man-pages/man2/munlock.2.html) map page.
    #[cfg(unix)]
    pub fn unlock(&self) -> std::io::Result<()> {
        self.inner.unlock()
    }

    /// Adjust the size of the memory mapping.
    ///
    /// This will try to resize the memory mapping in place. If
    /// [`RemapOptions::may_move`] is specified it will move the mapping if it
    /// could not resize in place, otherwise it will error.
    ///
    /// Only supported on Linux.
    ///
    /// See the [`mremap(2)`] man page.
    ///
    /// # Safety
    ///
    /// Resizing the memory mapping beyond the end of the mapped file will
    /// result in UB should you happen to access memory beyond the end of the
    /// file.
    ///
    /// [`mremap(2)`]: https://man7.org/linux/man-pages/man2/mremap.2.html
    #[cfg(target_os = "linux")]
    pub unsafe fn remap(&mut self, new_len: usize, options: RemapOptions) -> std::io::Result<()> {
        self.inner.remap(new_len, options)
    }
}

#[cfg(feature = "stable_deref_trait")]
unsafe impl stable_deref_trait::StableDeref for MmapMut {}

impl Deref for MmapMut {
    type Target = [u8];

    #[inline]
    fn deref(&self) -> &[u8] {
        unsafe { slice::from_raw_parts(self.inner.ptr(), self.inner.len()) }
    }
}

impl DerefMut for MmapMut {
    #[inline]
    fn deref_mut(&mut self) -> &mut [u8] {
        unsafe { slice::from_raw_parts_mut(self.inner.mut_ptr(), self.inner.len()) }
    }
}

impl AsRef<[u8]> for MmapMut {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.deref()
    }
}

impl AsMut<[u8]> for MmapMut {
    #[inline]
    fn as_mut(&mut self) -> &mut [u8] {
        self.deref_mut()
    }
}

impl fmt::Debug for MmapMut {
    fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
        fmt.debug_struct("MmapMut")
            .field("ptr", &self.as_ptr())
            .field("len", &self.len())
            .finish()
    }
}

/// Options for [`Mmap::remap`] and [`MmapMut::remap`].
#[derive(Copy, Clone, Default, Debug)]
#[cfg(target_os = "linux")]
pub struct RemapOptions {
    may_move: bool,
}

#[cfg(target_os = "linux")]
impl RemapOptions {
    /// Creates a mew set of options for resizing a memory map.
    pub fn new() -> Self {
        Self::default()
    }

    /// Controls whether the memory map can be moved if it is not possible to
    /// resize it in place.
    ///
    /// If false then the memory map is guaranteed to remain at the same
    /// address when being resized but attempting to resize will return an
    /// error if the new memory map would overlap with something else in the
    /// current process' memory.
    ///
    /// By default this is false.
    ///
    /// # `may_move` and `StableDeref`
    /// If the `stable_deref_trait` feature is enabled then [`Mmap`] and
    /// [`MmapMut`] implement `StableDeref`. `StableDeref` promises that the
    /// memory map dereferences to a fixed address, however, calling `remap`
    /// with `may_move` set may result in the backing memory of the mapping
    /// being moved to a new address. This may cause UB in other code
    /// depending on the `StableDeref` guarantees.
    pub fn may_move(mut self, may_move: bool) -> Self {
        self.may_move = may_move;
        self
    }

    pub(crate) fn into_flags(self) -> libc::c_int {
        if self.may_move {
            libc::MREMAP_MAYMOVE
        } else {
            0
        }
    }
}
