#![forbid(unsafe_code)]

//! High-level crate to extract the dependency trees embedded in binaries by [`cargo auditable`](https://crates.io/crates/cargo-auditable).
//!
//! Deserializes them to a JSON string or Rust data structures, at your option.
//!
//! ```rust, ignore
//! // Uses the default limits: 1GiB input file size, 8MiB audit data size
//! let info = audit_info_from_file(&PathBuf::from("path/to/file"), Default::default())?;
//! ```
//! Functions to load the data from a `Read` instance or from `&[u8]` are also provided.
//!
//! The supported formats are [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format),
//! [PE](https://en.wikipedia.org/wiki/Portable_Executable),
//! [Mach-O](https://en.wikipedia.org/wiki/Mach-O) and [WebAssembly](https://en.wikipedia.org/wiki/WebAssembly).
//!
//! If you need an even lower-level interface than the one provided by this crate,
//! use the [`auditable-extract`](http://docs.rs/auditable-extract/) and
//! [`auditable-serde`](http://docs.rs/auditable-serde/) crates.

use auditable_extract::raw_auditable_data;
#[cfg(feature = "serde")]
use auditable_serde::VersionInfo;
use miniz_oxide::inflate::decompress_to_vec_zlib_with_limit;
use std::fs::File;
use std::io::{BufRead, BufReader, Read};
use std::path::Path;

mod error;

pub use crate::error::*;

/// Loads audit info from the specified binary compiled with `cargo auditable`.
///
/// The entire file is loaded into memory. The RAM usage limit can be configured using the [`Limits`] struct.
///
/// ```rust, ignore
/// // Uses the default limits: 1GiB input file size, 8MiB audit data size
/// let info = audit_info_from_file(&PathBuf::from("path/to/file"), Default::default())?;
/// ```
///
/// The data is validated to only have a single root package and not contain any circular dependencies.
#[cfg(feature = "serde")]
pub fn audit_info_from_file(path: &Path, limits: Limits) -> Result<VersionInfo, Error> {
    Ok(serde_json::from_str(&json_from_file(path, limits)?)?)
}

/// Extracts the audit data from the specified binary and returns the JSON string.
/// This is useful if you want to forward the data somewhere instead of parsing it to Rust data structures.
///
/// If you want to obtain the Zlib-compressed data instead,
/// use the [`auditable-extract`](https://docs.rs/auditable-extract/) crate directly.
pub fn json_from_file(path: &Path, limits: Limits) -> Result<String, Error> {
    let file = File::open(path)?;
    let mut reader = BufReader::new(file);
    json_from_reader(&mut reader, limits)
}

/// Loads audit info from the binary loaded from an arbitrary reader, e.g. the standard input.
///
/// ```rust, ignore
/// let stdin = io::stdin();
/// let mut handle = stdin.lock();
/// // Uses the default limits: 1GiB input file size, 8MiB audit data size
/// let info = audit_info_from_reader(&mut handle, Default::default())?;
/// ```
///
/// The data is validated to only have a single root package and not contain any circular dependencies.
#[cfg(feature = "serde")]
pub fn audit_info_from_reader<T: BufRead>(
    reader: &mut T,
    limits: Limits,
) -> Result<VersionInfo, Error> {
    Ok(serde_json::from_str(&json_from_reader(reader, limits)?)?)
}

/// Extracts the audit data and returns the JSON string.
/// This is useful if you want to forward the data somewhere instead of parsing it to Rust data structures.
///
/// If you want to obtain the Zlib-compressed data instead,
/// use the [`auditable-extract`](https://docs.rs/auditable-extract/) crate directly.
pub fn json_from_reader<T: BufRead>(reader: &mut T, limits: Limits) -> Result<String, Error> {
    let compressed_data = get_compressed_audit_data(reader, limits)?;
    let decompressed_data =
        decompress_to_vec_zlib_with_limit(&compressed_data, limits.decompressed_json_size)
            .map_err(DecompressError::from_miniz)?;
    Ok(String::from_utf8(decompressed_data)?)
}

// Factored into its own function for ease of unit testing,
// and also so that the large allocation of the input file is dropped
// before we start decompressing the data to minimize peak memory usage
fn get_compressed_audit_data<T: BufRead>(reader: &mut T, limits: Limits) -> Result<Vec<u8>, Error> {
    // In case you're wondering why the check for the limit is weird like that:
    // When .take() returns EOF, it doesn't tell you if that's because it reached the limit
    // or because the underlying reader ran out of data.
    // And we need to return an error when the reader is over limit, else we'll truncate the audit data.
    // So it would be reasonable to run `into_inner()` and check if that reader has any data remaining...
    // But readers can return EOF sporadically - a reader may return EOF,
    // then get more data and return bytes again instead of EOF!
    // So instead we read as many bytes as the limit allows, plus one.
    // If we've read the limit-plus-one bytes, that means the underlying reader was at least one byte over the limit.
    // That way we avoid any time-of-check/time-of-use issues.
    let incremented_limit = u64::saturating_add(limits.input_file_size as u64, 1);
    let mut f = reader.take(incremented_limit);
    let mut input_binary = Vec::new();
    f.read_to_end(&mut input_binary)?;
    if input_binary.len() as u64 == incremented_limit {
        Err(Error::InputLimitExceeded)?
    }
    let compressed_audit_data = raw_auditable_data(&input_binary)?;
    if compressed_audit_data.len() > limits.decompressed_json_size {
        Err(Error::OutputLimitExceeded)?;
    }
    Ok(compressed_audit_data.to_owned())
}

/// The input slice should contain the entire binary.
/// This function is useful if you have already loaded the binary to memory, e.g. via memory-mapping.
#[cfg(feature = "serde")]
pub fn audit_info_from_slice(
    input_binary: &[u8],
    decompressed_json_size_limit: usize,
) -> Result<VersionInfo, Error> {
    Ok(serde_json::from_str(&json_from_slice(
        input_binary,
        decompressed_json_size_limit,
    )?)?)
}

/// The input slice should contain the entire binary.
/// This function is useful if you have already loaded the binary to memory, e.g. via memory-mapping.
///
/// Returns the decompressed audit data.
/// This is useful if you want to forward the data somewhere instead of parsing it to Rust data structures.
///
/// If you want to obtain the Zlib-compressed data instead,
/// use the [`auditable-extract`](https://docs.rs/auditable-extract/) crate directly.
pub fn json_from_slice(
    input_binary: &[u8],
    decompressed_json_size_limit: usize,
) -> Result<String, Error> {
    let compressed_audit_data = raw_auditable_data(input_binary)?;
    if compressed_audit_data.len() > decompressed_json_size_limit {
        Err(Error::OutputLimitExceeded)?;
    }
    let decompressed_data =
        decompress_to_vec_zlib_with_limit(compressed_audit_data, decompressed_json_size_limit)
            .map_err(DecompressError::from_miniz)?;
    Ok(String::from_utf8(decompressed_data)?)
}

/// Protects against [denial-of-service attacks](https://en.wikipedia.org/wiki/Denial-of-service_attack)
/// via infinite input streams or [zip bombs](https://en.wikipedia.org/wiki/Zip_bomb),
/// which would otherwise use up all your memory and crash your machine.
///
/// If the limit is exceeded, an error is returned and no further deserialization is attempted.
///
/// The default limits are **1 GiB** for the `input_file_size` and **8 MiB** for `decompressed_json_size`.
///
/// Note that the `decompressed_json_size` is only enforced on the level of the *serialized* JSON, i.e. a string.
/// We do not enforce that `serde_json` does not consume more memory when deserializing JSON to Rust data structures.
/// Unfortunately Rust does not provide APIs for that.
#[derive(Copy, Clone, Eq, PartialEq, Hash)]
pub struct Limits {
    pub input_file_size: usize,
    pub decompressed_json_size: usize,
}

impl Default for Limits {
    fn default() -> Self {
        Self {
            input_file_size: 1024 * 1024 * 1024,     // 1GiB
            decompressed_json_size: 1024 * 1024 * 8, // 8MiB
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn input_file_limits() {
        let limits = Limits {
            input_file_size: 128,
            decompressed_json_size: 99999,
        };
        let fake_data = vec![0; 1024];
        let mut reader = std::io::Cursor::new(fake_data);
        let result = get_compressed_audit_data(&mut reader, limits);
        assert!(result.is_err());
        assert!(result
            .unwrap_err()
            .to_string()
            .contains("The input file is too large"));
    }
}
