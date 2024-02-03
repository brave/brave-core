//! Cargo.lock-related utilities

use rustsec::{Error, ErrorKind};
use std::{
    path::{Path, PathBuf},
    process::Command,
};

/// Name of `Cargo.lock`
const CARGO_LOCK_FILE: &str = "Cargo.lock";

/// Tries to locate the lockfile at the specified file path. If it's missing, tries to generate it from `Cargo.toml`.
/// Defaults to `Cargo.lock` in the current directory if passed `None` as the path.
pub fn locate_or_generate(maybe_lockfile_path: Option<&Path>) -> rustsec::Result<PathBuf> {
    match maybe_lockfile_path {
        Some(p) => Ok(p.into()),
        None => {
            let path = Path::new(CARGO_LOCK_FILE);
            if !path.exists() && Path::new("Cargo.toml").exists() {
                generate()?;
            }
            Ok(path.into())
        }
    }
}

/// Run `cargo generate-lockfile`
pub fn generate() -> rustsec::Result<()> {
    let status = Command::new("cargo").arg("generate-lockfile").status();

    if let Err(e) = status {
        return Err(Error::new(
            ErrorKind::Io,
            &format!("couldn't run `cargo generate-lockfile`: {}", e),
        ));
    }
    let status = status.unwrap();

    if !status.success() {
        let msg = match status.code() {
            Some(code) => format!(
                "non-zero exit status running `cargo generate-lockfile`: {}",
                code
            ),
            _ => "no exit status running `cargo generate-lockfile`!".to_string(),
        };

        return Err(Error::new(ErrorKind::Io, &msg));
    }
    Ok(())
}
