#![cfg_attr(docsrs, feature(doc_cfg))]

//! An extension trait for safely dropping [I/O writers](std::io::Write)
//! such as [`File`](std::fs::File) and
//! [`BufWriter`](std::io::BufWriter). Specifically, it is for I/O
//! writers which may contain a resource handle (such as a raw file
//! descriptor), and where the automatic process of closing this handle
//! during drop may generate an unseen I/O error. Using this trait
//! (called [`Close`]) these errors can be seen and dealt with.
//!
//! In the case of Linux [the man page for
//! close(2)](https://linux.die.net/man/2/close) has the following to
//! say:
//!
//! > Not checking the return value of close() is a common but
//! > nevertheless serious programming error. It is quite possible that
//! > errors on a previous write(2) operation are first reported at the
//! > final close(). Not checking the return value when closing the file
//! > may lead to silent loss of data. This can especially be observed
//! > with NFS and with disk quota.
//!
//! Implementations of [`Close`] are provided for most standard library
//! I/O writers and (optionally) for a selection of I/O writers defined
//! in external crates.
//!
//! # BufWriter example
//!
//! ```
//! use std::io::{BufWriter, Result, Write};
//! use io_close::Close;
//!
//! fn main() -> Result<()> {
//!     let data = b"hello world";
//!     let mut buffer = BufWriter::new(tempfile::tempfile()?);
//!     buffer.write_all(data)?;
//!     buffer.close()?; // safely drop buffer and its contained File
//!     Ok(())
//! }
//! ```
//!
//! # Optional implementations
//!
//! Optional implementations of [`Close`] are provided for the following
//! I/O writers defined in external crates, enabled through cargo features:
//!
//! - [`os_pipe::PipeWriter`] (feature: `os_pipe`)

use std::io::{Error, Result, Write};

pub mod fs;

/// An extension trait for safely dropping I/O writers.
pub trait Close: Write {
    /// Drops an I/O writer and closes any resource handle contained
    /// inside (such as a raw file descriptor). Ensures that I/O errors
    /// resulting from closing a handle are not ignored. The writer is
    /// flushed before any handle is closed. If any errors occur during
    /// flushing or closing the first such error is returned.
    fn close(self) -> Result<()>;
}

// MACRO DEFINITIONS

macro_rules! unix_impl_close_raw_fd {
    ($ty:ty, "std" $(,$lt:lifetime)* $(,$id:ident)*) => {
        unix_impl_close_raw_fd!($ty, "unix" $(,$lt)* $(,$id)*);
    };
    ($ty:ty, $ft_fm:literal $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(unix)]
        #[cfg(any(feature = $ft_fm, target_family = $ft_fm))]
        #[cfg_attr(all(docsrs, feature = $ft_fm), doc(cfg(feature = $ft_fm)))]
        impl<$($lt,)* $($id,)*> Close for $ty {
            /// Drops an I/O writer containing a raw file descriptor.
            fn close(mut self) -> Result<()> {
                use std::io::ErrorKind;
                use std::os::unix::io::IntoRawFd;

                self.flush()?;
                let fd = self.into_raw_fd();
                let rv = unsafe { libc::close(fd) };
                if rv != -1 {
                    Ok(())
                } else {
                    match Error::last_os_error() {
                        e if e.kind() == ErrorKind::Interrupted => Ok(()),
                        e => Err(e),
                    }
                }
            }
        }
    };
}

macro_rules! windows_impl_close_raw_handle {
    ($ty:ty, "std" $(,$lt:lifetime)* $(,$id:ident)*) => {
        windows_impl_close_raw_handle!($ty, "windows" $(,$lt)* $(,$id)*);
    };
    ($ty:ty, $ft_fm:literal $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(windows)]
        #[cfg(any(feature = $ft_fm, target_family = $ft_fm))]
        #[cfg_attr(all(docsrs, feature = $ft_fm), doc(cfg(feature = $ft_fm)))]
        impl<$($lt,)* $($id,)*> Close for $ty {
            /// Drops an I/O writer containing a raw handle.
            fn close(mut self) -> Result<()> {
                use std::os::windows::io::IntoRawHandle;
                use winapi::um::handleapi;

                self.flush()?;
                let handle = self.into_raw_handle();
                let rv = unsafe { handleapi::CloseHandle(handle) };
                if rv != 0 {
                    Ok(())
                } else {
                    Err(Error::last_os_error())
                }
            }
        }
    };
}

macro_rules! windows_impl_close_raw_socket {
    ($ty:ty, "std" $(,$lt:lifetime)* $(,$id:ident)*) => {
        windows_impl_close_raw_socket!($ty, "windows" $(,$lt)* $(,$id)*);
    };
    ($ty:ty, $ft_fm:literal $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(windows)]
        #[cfg(any(feature = $ft_fm, target_family = $ft_fm))]
        #[cfg_attr(all(docsrs, feature = $ft_fm), doc(cfg(feature = $ft_fm)))]
        impl<$($lt,)* $($id,)*> Close for $ty {
            /// Drops an I/O writer containing a raw socket.
            fn close(mut self) -> Result<()> {
                use std::convert::TryInto;
                use std::os::windows::io::IntoRawSocket;
                use winapi::um::winsock2;

                self.flush()?;
                let socket = self.into_raw_socket().try_into().unwrap();
                let rv = unsafe { winsock2::closesocket(socket) };
                if rv == 0 {
                    Ok(())
                } else {
                    Err(Error::from_raw_os_error(unsafe {
                        winsock2::WSAGetLastError()
                    }))
                }
            }
        }
    };
}

macro_rules! impl_close_no_error_no_flush {
    ($ty:ty, "std" $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(unix)]
        impl_close_no_error_no_flush!($ty, "unix" $(,$lt)* $(,$id)*);
        #[cfg(windows)]
        impl_close_no_error_no_flush!($ty, "windows" $(,$lt)* $(,$id)*);
    };
    ($ty:ty, $ft_fm:literal $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(any(unix, windows))]
        #[cfg(any(feature = $ft_fm, target_family = $ft_fm))]
        #[cfg_attr(all(docsrs, feature = $ft_fm), doc(cfg(feature = $ft_fm)))]
        impl<$($lt,)* $($id,)*> Close for $ty {
            /// Drops an I/O writer for which `close()` never produces
            /// an error, and for which flushing is unnecessary.
            #[inline]
            fn close(self) -> Result<()> {
                Ok(())
            }
        }
    };
}

macro_rules! impl_close_into_inner {
    ($ty:ty, "std" $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(unix)]
        impl_close_into_inner!($ty, "unix" $(,$lt)* $(,$id)*);
        #[cfg(windows)]
        impl_close_into_inner!($ty, "windows" $(,$lt)* $(,$id)*);
    };
    ($ty:ty, $ft_fm:literal $(,$lt:lifetime)* $(,$id:ident)*) => {
        #[cfg(any(unix, windows))]
        #[cfg(any(feature = $ft_fm, target_family = $ft_fm))]
        #[cfg_attr(all(docsrs, feature = $ft_fm), doc(cfg(feature = $ft_fm)))]
        impl<$($lt,)* W: Close, $($id,)*> Close for $ty {
            /// Drops an I/O writer which can be unwrapped using
            /// `into_inner()` to return an underlying writer.
            fn close(self) -> Result<()> {
                self.into_inner()?.close()
            }
        }
    };
}

// IMPLEMENTATIONS
//
// In the below macro implementations for Close the macro parameters use
// the following system:
//
//     1st paramater: the type that Close is being implemented for
//     2nd parameter: either "std", or a feature name
//     3rd, 4th, etc. parameters: additional generic arguments for type
//
// If the 2nd parameter is a feature name then the implementation will
// be conditionally compiled only when that feature is present.

unix_impl_close_raw_fd!(std::fs::File, "std");
unix_impl_close_raw_fd!(std::net::TcpStream, "std");
unix_impl_close_raw_fd!(std::os::unix::net::UnixStream, "std");
unix_impl_close_raw_fd!(std::process::ChildStdin, "std");
unix_impl_close_raw_fd!(os_pipe::PipeWriter, "os_pipe");

windows_impl_close_raw_handle!(std::fs::File, "std");
windows_impl_close_raw_socket!(std::net::TcpStream, "std");
windows_impl_close_raw_handle!(std::process::ChildStdin, "std");
windows_impl_close_raw_handle!(os_pipe::PipeWriter, "os_pipe");

impl_close_no_error_no_flush!(&mut [u8], "std");
impl_close_no_error_no_flush!(std::io::Cursor<&mut Vec<u8>>, "std");
impl_close_no_error_no_flush!(std::io::Cursor<&mut [u8]>, "std");
impl_close_no_error_no_flush!(std::io::Cursor<Box<[u8]>>, "std");
impl_close_no_error_no_flush!(std::io::Cursor<Vec<u8>>, "std");
impl_close_no_error_no_flush!(std::io::Sink, "std");
impl_close_no_error_no_flush!(Vec<u8>, "std");

impl_close_into_inner!(std::io::BufWriter<W>, "std");
impl_close_into_inner!(std::io::LineWriter<W>, "std");
