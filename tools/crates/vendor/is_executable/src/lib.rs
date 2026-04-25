/*!
Is there an executable file at the given path?

[![](https://docs.rs/is_executable/badge.svg)](https://docs.rs/is_executable/) [![](http://meritbadge.herokuapp.com/is_executable) ![](https://img.shields.io/crates/d/is_executable.png)](https://crates.io/crates/is_executable) [![Unix Build Status](https://travis-ci.org/fitzgen/is_executable.png?branch=master)](https://travis-ci.org/fitzgen/is_executable) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/fitzgen/is_executable?branch=master&svg=true)](https://ci.appveyor.com/project/fitzgen/is-executable)

A small helper function which determines whether or not the given path points to
an executable file. If there is no file at the given path, or the file is not
executable, then `false` is returned. When there is a file and the file is
executable, then `true` is returned.

This crate works on both unix-based operating systems (mac, linux, freebsd, etc.) and Windows.

The API comes in two flavors:

1. An extension trait to add an `is_executable` method on `std::path::Path`:

    ```rust
    use std::path::Path;

    use is_executable::IsExecutable;

    fn main() {
        let path = Path::new("some/path/to/a/file");

        // Determine if `path` is executable.
        if path.is_executable() {
            println!("The path is executable!");
        } else {
            println!("The path is _not_ executable!");
        }
    }
    ```

2. For convenience, a standalone `is_executable` function, which takes any
`AsRef<Path>`:

    ```rust
    use std::path::Path;

    use is_executable::is_executable;

    fn main() {
        let path = Path::new("some/path/to/a/file");

        // Determine if `path` is executable.
        if is_executable(&path) {
            println!("The path is executable!");
        } else {
            println!("The path is _not_ executable!");
        }
    }
    ```
 */

#[cfg(target_os = "windows")]
extern crate winapi;

use std::path::Path;

/// Returns `true` if there is a file at the given path and it is
/// executable. Returns `false` otherwise.
///
/// See the module documentation for details.
pub fn is_executable<P>(path: P) -> bool
where
    P: AsRef<Path>,
{
    path.as_ref().is_executable()
}

/// An extension trait for `std::fs::Path` providing an `is_executable` method.
///
/// See the module documentation for examples.
pub trait IsExecutable {
    /// Returns `true` if there is a file at the given path and it is
    /// executable. Returns `false` otherwise.
    ///
    /// See the module documentation for details.
    fn is_executable(&self) -> bool;
}

#[cfg(unix)]
mod unix {
    use std::os::unix::fs::PermissionsExt;
    use std::path::Path;

    use super::IsExecutable;

    impl IsExecutable for Path {
        fn is_executable(&self) -> bool {
            let metadata = match self.metadata() {
                Ok(metadata) => metadata,
                Err(_) => return false,
            };
            let permissions = metadata.permissions();
            permissions.mode() & 0o111 != 0
        }
    }
}

#[cfg(target_os = "windows")]
mod windows {
    use std::os::windows::ffi::OsStrExt;
    use std::path::Path;

    use winapi::ctypes::{c_ulong, wchar_t};
    use winapi::um::winbase::GetBinaryTypeW;

    use super::IsExecutable;

    impl IsExecutable for Path {
        fn is_executable(&self) -> bool {
            let windows_string = self
                .as_os_str()
                .encode_wide()
                .chain(Some(0))
                .collect::<Vec<wchar_t>>();
            let windows_string_ptr = windows_string.as_ptr();

            let mut binary_type: c_ulong = 42;
            let binary_type_ptr = &mut binary_type as *mut c_ulong;

            let ret = unsafe { GetBinaryTypeW(windows_string_ptr, binary_type_ptr) };
            if binary_type_ptr.is_null() {
                return false;
            }
            if ret != 0 {
                let binary_type = unsafe { *binary_type_ptr };
                match binary_type {
                    0   // A 32-bit Windows-based application
                    | 1 // An MS-DOS-based application
                    | 2 // A 16-bit Windows-based application
                    | 3 // A PIF file that executes an MS-DOS-based application
                    | 4 // A POSIX – based application
                    | 5 // A 16-bit OS/2-based application
                    | 6 // A 64-bit Windows-based application
                    => return true,
                    _ => (),
                }
            }

            false
        }
    }
}
