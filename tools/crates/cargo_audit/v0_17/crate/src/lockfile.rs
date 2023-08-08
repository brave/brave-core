//! Cargo.lock-related utilities

use rustsec::{Error, ErrorKind};
use std::process::Command;

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
