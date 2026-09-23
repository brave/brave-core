//! Extracts the list of dependencies from a binary file

use std::str::FromStr;

use crate::{Error, ErrorKind};
use auditable_serde::VersionInfo;
use cargo_lock::{Dependency, Lockfile, Package};

use crate::binary_scanning::BinaryFormat;

/// The default file size limit is 8MB
const DEFAULT_FILE_SIZE_LIMIT: usize = 8 * 1024 * 1024;

/// Dependencies recovered from scanning a compiled Rust executable
pub enum BinaryReport {
    /// Full dependency list embedded by `cargo auditable`
    Complete(Lockfile),
    /// Partially recovered dependencies from panic messages
    Incomplete(Lockfile),
    /// No data found whatsoever, probably not a Rust executable
    None,
}

/// Load the dependency tree from a compiled Rust executable.
///
/// Recovers the precise dependency list if the binary is built with [`cargo auditable`](https://crates.io/crates/cargo-auditable).
/// Failing that, recovers as many crates as possible from panic messages (using [quitters]).
///
/// If `audit_data_size_limit` is set to `None`, the limit defaults to 8MB.
pub fn load_deps_from_binary(
    file_contents: &[u8],
    audit_data_size_limit: Option<usize>,
) -> crate::Result<(BinaryFormat, BinaryReport)> {
    let format = detect_format(file_contents);
    let file_size_limit = match audit_data_size_limit {
        Some(size) => size,
        None => DEFAULT_FILE_SIZE_LIMIT,
    };
    let version_info = auditable_info::audit_info_from_slice(file_contents, file_size_limit);

    use auditable_info::Error::*; // otherwise rustfmt makes the matches multiline and unreadable
    match version_info {
        Ok(json_struct) => Ok((
            format,
            BinaryReport::Complete(lockfile_from_version_info_json(&json_struct)?),
        )),
        Err(e) => match e {
            NoAuditData => {
                if let Some(deps) = deps_from_panic_messages(file_contents) {
                    Ok((format, BinaryReport::Incomplete(deps)))
                } else {
                    Ok((format, BinaryReport::None))
                }
            }
            // The error handling boilerplate is in here instead of the `rustsec` crate because as of this writing
            // the public APIs of the crates involved are still somewhat unstable,
            // and this way we don't expose the error types in any public APIs
            Io(_) => Err(Error::with_source(
                ErrorKind::Io,
                "could not extract dependencies from binary".to_string(),
                e,
            )),
            // Everything else is just Parse, but we enumerate them explicitly in case variant list changes
            InputLimitExceeded | OutputLimitExceeded | BinaryParsing(_) | Decompression(_)
            | Json(_) | Utf8(_) => Err(Error::with_source(
                ErrorKind::Parse,
                "could not extract dependencies from binary".to_string(),
                e,
            )),
        },
    }
}

fn detect_format(data: &[u8]) -> BinaryFormat {
    match binfarce::detect_format(data) {
        binfarce::Format::Unknown => {
            // binfarce doesn't detect WASM
            if data.starts_with(b"\0asm") {
                BinaryFormat::Wasm
            } else {
                BinaryFormat::Unknown
            }
        }
        known_format => known_format.into(),
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

/// Recover a [`Lockfile`] from a [`VersionInfo`] struct.
fn lockfile_from_version_info_json(input: &VersionInfo) -> Result<Lockfile, cargo_lock::Error> {
    let mut root_package: Option<Package> = None;
    let mut packages: Vec<Package> = Vec::new();
    for pkg in input.packages.iter() {
        let lock_pkg = Package {
            name: cargo_lock::package::Name::from_str(&pkg.name)?,
            version: pkg.version.clone(),
            checksum: None,
            dependencies: {
                let result: Result<Vec<_>, _> = pkg
                    .dependencies
                    .iter()
                    .map(|i| {
                        let package =
                            input
                                .packages
                                .get(*i)
                                .ok_or(cargo_lock::Error::Parse(format!(
                                    "There is no dependency with index {i} in the input JSON"
                                )))?;

                        Result::<_, cargo_lock::Error>::Ok(Dependency {
                            name: cargo_lock::package::Name::from_str(package.name.as_str())?,
                            version: package.version.clone(),
                            source: source_from_json(&package.source),
                        })
                    })
                    .collect();
                result?
            },
            replace: None,
            source: source_from_json(&pkg.source),
        };
        if pkg.root {
            if root_package.is_some() {
                return Err(cargo_lock::Error::Parse(
                    "More than one root package specified in JSON!".to_string(),
                ));
            }
            root_package = Some(lock_pkg.clone());
        }
        packages.push(lock_pkg);
    }
    Ok(Lockfile {
        version: cargo_lock::ResolveVersion::V2,
        packages,
        root: root_package,
        metadata: std::collections::BTreeMap::new(),
        patch: cargo_lock::Patch { unused: Vec::new() },
    })
}

fn source_from_json(source: &auditable_serde::Source) -> Option<cargo_lock::SourceId> {
    match source {
        auditable_serde::Source::CratesIo => Some(
            cargo_lock::package::SourceId::from_url(
                "registry+https://github.com/rust-lang/crates.io-index",
            )
            .unwrap(),
        ),
        _ => None, // we don't store enough info about other sources to reconstruct the URL
    }
}
