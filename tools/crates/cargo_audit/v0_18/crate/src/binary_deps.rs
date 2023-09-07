//! Extracts dependencies from binary files, using one of two ways:
//! 1. Recovers the dependency list embedded by `cargo auditable` (using `auditable-info`)
//! 2. Failing that, recovers as many crates as possible from panic messages (using `quitters`)

use std::{path::Path, str::FromStr};

use cargo_lock::{Lockfile, Package};
use rustsec::{Error, ErrorKind};

use crate::binary_format::BinaryFormat;

pub enum BinaryReport {
    /// Full dependency list embedded by `cargo auditable`
    Complete(Lockfile),
    /// Partially recovered dependencies from panic messages
    Incomplete(Lockfile),
    /// No data found whatsoever, probably not a Rust executable
    None,
}

/// Load the dependency tree from a binary file
pub fn load_deps_from_binary(binary_path: &Path) -> rustsec::Result<(BinaryFormat, BinaryReport)> {
    // TODO: input size limit
    let file_contents = std::fs::read(binary_path)?;
    let format = binfarce::detect_format(&file_contents).into();
    let stuff = auditable_info::audit_info_from_slice(&file_contents, 8 * 1024 * 1024);

    use auditable_info::Error::*; // otherwise rustfmt makes the matches multiline and unreadable
    match stuff {
        Ok(json_struct) => Ok((
            format,
            BinaryReport::Complete(cargo_lock::Lockfile::try_from(&json_struct)?),
        )),
        Err(e) => match e {
            NoAuditData => {
                if let Some(deps) = deps_from_panic_messages(&file_contents) {
                    Ok((format, BinaryReport::Incomplete(deps)))
                } else {
                    Ok((format, BinaryReport::None))
                }
            }
            // The error handling boilerplate is in here instead of the `rustsec` crate because as of this writing
            // the public APIs of the crates involved are still somewhat unstable,
            // and this way we don't expose the error types in any public APIs
            Io(_) => Err(Error::new(ErrorKind::Io, &e.to_string())),
            // Everything else is just Parse, but we enumerate them explicitly in case variant list changes
            InputLimitExceeded | OutputLimitExceeded | BinaryParsing(_) | Decompression(_)
            | Json(_) | Utf8(_) => Err(Error::new(ErrorKind::Parse, &e.to_string())),
        },
    }
}

fn deps_from_panic_messages(data: &[u8]) -> Option<Lockfile> {
    let deps = quitters::versions(data);
    if !deps.is_empty() {
        let packages: Vec<Package> = deps.into_iter().map(to_package).collect();
        Some(Lockfile {
            version: cargo_lock::ResolveVersion::V2,
            packages,
            root: None,
            metadata: Default::default(),
            patch: Default::default(),
        })
    } else {
        None
    }
}

// matches https://docs.rs/cargo-lock/8.0.2/src/cargo_lock/package/source.rs.html#19
// to signal crates.io to the `cargo-lock` crate
const CRATES_IO_INDEX: &str = "registry+https://github.com/rust-lang/crates.io-index";

fn to_package(quitter: (&str, cargo_lock::Version)) -> Package {
    Package {
        // The `quitters` crate already ensures the name is valid, so we can just `.unwrap()` here
        name: cargo_lock::Name::from_str(quitter.0).unwrap(),
        version: quitter.1,
        // we can't know the exact registry, but by default `cargo audit` will
        // only scan crates from crates.io, so assume they're from there
        source: Some(cargo_lock::package::SourceId::from_url(CRATES_IO_INDEX).unwrap()),
        checksum: None,
        dependencies: Vec::new(),
        replace: None,
    }
}
