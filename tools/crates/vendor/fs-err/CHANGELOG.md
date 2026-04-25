# fs-err Changelog

## 3.2.2

* Add wrappers for `File::set_modified` and `File::set_times` ([#84](https://github.com/andrewhickman/fs-err/pull/84))

## 3.2.1

* Rename parameters of `symlink`, `soft_link` and `hard_link` functions to match `std` ([#83](https://github.com/andrewhickman/fs-err/pull/83))

## 3.2.0

* Introduce `debug` and `debug_tokio` feature. Debug filesystem errors faster by exposing more information ([#81](https://github.com/andrewhickman/fs-err/pull/81)). Without this feature on, errors might look like this:

  ```
  failed to open file `file.txt`: The system cannot find the file specified. (os error 2)
  ```

  With this feature on, it will include additional information. For example:

  ```
  failed to open file `file.txt`: The system cannot find the file specified. (os error 2)

  Path does not exist `file.txt`
  - Absolute path `/path/to/dir/file.txt`
  - Missing `file.txt` from parent directory:
    `/path/to/dir`
      └── `file.md`
      └── `different.txt`
  ```

  It's suggested to enable this feature in `dev-dependencies` for security and performance reasons.

## 3.1.3

* Add wrappers for `std::fs::exists` and `tokio::fs::try_exists` ([#77](https://github.com/andrewhickman/fs-err/pull/77))

## 3.1.2

* Added wrappers for locking methods added to `File` in Rust 1.89 ([#75](https://github.com/andrewhickman/fs-err/pull/75))

## 3.1.1

* Added `File::into_file` and `File::into_path` ([#73](https://github.com/andrewhickman/fs-err/pull/73))

## 3.1.0

* Added new wrappers for `create_new` and `options` functions on `File` ([#69](https://github.com/andrewhickman/fs-err/pull/69))

## 3.0.0

* Error messages now include the original message from `std::io::Error` by default ([#60](https://github.com/andrewhickman/fs-err/pull/60)). Previously this was exposed through the [`Error::source()`](https://doc.rust-lang.org/stable/std/error/trait.Error.html#method.source) method. For example, previously a message would look like:

  ```
  failed to open file `file.txt`
  ```

  and you would have to remember to print the source, or use a library like `anyhow` to print the full chain of source errors. The new error message includes the cause by default

  ```
  failed to open file `file.txt`: The system cannot find the file specified. (os error 2)
  ```

  Note that the original error is no longer exposed though [`Error::source()`](https://doc.rust-lang.org/stable/std/error/trait.Error.html#method.source) by default. If you need access to it, you can restore the previous behaviour with the `expose_original_error` feature flag.

* The `io_safety` feature flag has been removed, and this functionality is now always enabled on Rust versions which support it (1.63.0 and greater).

* Removed deprecated APIs: `File::from_options`, `tokio::symlink`

## 2.11.0

* Added the first line of the standard library documentation to each function's rustdocs, to make them more useful in IDEs ([#50](https://github.com/andrewhickman/fs-err/issues/45))
* Fixed the wrapper for `tokio::fs::symlink_dir()` on Windows being incorrectly named `symlink`. The old function is now deprecated and will be removed in the next breaking release.

## 2.10.0

* Add `fs_err_try_exists` to `std::path::Path` via extension trait. This feature requires Rust 1.63 or later. ([#48](https://github.com/andrewhickman/fs-err/pull/48))

## 2.9.0

* Add wrappers for [`tokio::fs`](https://docs.rs/tokio/latest/tokio/fs/index.html) ([#40](https://github.com/andrewhickman/fs-err/pull/40)).

## 2.8.1

* Fixed docs.rs build

## 2.8.0

* Implement I/O safety traits (`AsFd`/`AsHandle`, `Into<OwnedFd>`/`Into<OwnedHandle>`) for file. This feature requires Rust 1.63 or later and is gated behind the `io_safety` feature flag. ([#39](https://github.com/andrewhickman/fs-err/pull/39))

## 2.7.0

* Implement `From<fs_err::File> for std::fs::File` ([#38](https://github.com/andrewhickman/fs-err/pull/38))

## 2.6.0

* Added [`File::into_parts`](https://docs.rs/fs-err/2.6.0/fs_err/struct.File.html#method.into_parts) and [`File::file_mut`](https://docs.rs/fs-err/2.6.0/fs_err/struct.File.html#method.file_mut) to provide more access to the underlying `std::fs::File`.
* Fixed some typos in documention ([#33](https://github.com/andrewhickman/fs-err/pull/33))

## 2.5.0
* Added `symlink` for unix platforms
* Added `symlink_file` and `symlink_dir` for windows
* Implemented os-specific extension traits for `File`
  - `std::os::unix::io::{AsRawFd, IntoRawFd}`
  - `std::os::windows::io::{AsRawHandle, IntoRawHandle}`
  - Added trait wrappers for `std::os::{unix, windows}::fs::FileExt` and implemented them for `fs_err::File`
* Implemented os-specific extension traits for `OpenOptions`
  - Added trait wrappers for `std::os::{unix, windows}::fs::OpenOptionsExt` and implemented them for `fs_err::OpenOptions`
* Improved compile times by converting arguments early and forwarding only a small number of types internally. There will be a slight performance hit only in the error case.
* Reduced trait bounds on generics from `AsRef<Path> + Into<PathBuf>` to either `AsRef<Path>` or `Into<PathBuf>`, making the functions more general.

## 2.4.0
* Added `canonicalize`, `hard link`, `read_link`, `rename`, `symlink_metadata` and `soft_link`. ([#25](https://github.com/andrewhickman/fs-err/pull/25))
* Added aliases to `std::path::Path` via extension trait ([#26](https://github.com/andrewhickman/fs-err/pull/26))
* Added `OpenOptions` ([#27](https://github.com/andrewhickman/fs-err/pull/27))
* Added `set_permissions` ([#28](https://github.com/andrewhickman/fs-err/pull/28))

## 2.3.0
* Added `create_dir` and `create_dir_all`. ([#19](https://github.com/andrewhickman/fs-err/pull/19))
* Added `remove_file`, `remove_dir`, and `remove_dir_all`. ([#16](https://github.com/andrewhickman/fs-err/pull/16))

## 2.2.0
* Added `metadata`. ([#15](https://github.com/andrewhickman/fs-err/pull/15))

## 2.1.0
* Updated crate-level documentation. ([#8](https://github.com/andrewhickman/fs-err/pull/8))
* Added `read_dir`, `ReadDir`, and `DirEntry`. ([#9](https://github.com/andrewhickman/fs-err/pull/9))

## 2.0.1 (2020-02-22)
* Added `copy`. ([#7](https://github.com/andrewhickman/fs-err/pull/7))

## 2.0.0 (2020-02-19)
* Removed custom error type in favor of `std::io::Error`. ([#2](https://github.com/andrewhickman/fs-err/pull/2))

## 1.0.1 (2020-02-15)
* Fixed bad documentation link in `Cargo.toml`.

## 1.0.0 (2020-02-15)
* No changes from 0.1.2.

## 0.1.2 (2020-02-10)
* Added `Error::cause` implementation for `fs_err::Error`.

## 0.1.1 (2020-02-05)
* Added wrappers for `std::fs::*` functions.

## 0.1.0 (2020-02-02)
* Initial release, containing a wrapper around `std::fs::File`.
