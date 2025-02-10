//! Automatically attempt to fix vulnerable dependencies
//!
//! This module is **experimental**, and its behavior may change in the future.

use crate::vulnerability::Vulnerability;
use cargo_lock::{Lockfile, Package};
use std::path::{Path, PathBuf};
use std::process::Command;

/// Auto-fixer for vulnerable dependencies
#[cfg_attr(docsrs, doc(cfg(feature = "fix")))]
pub struct Fixer {
    lockfile: Lockfile,
    manifest_path: Option<PathBuf>,
    path_to_cargo: Option<PathBuf>,
}

impl Fixer {
    /// Create a new [`Fixer`] for the given `Cargo.lock` file
    ///
    /// `path_to_cargo` defaults to `cargo`, resolved in your `$PATH`.
    ///
    /// If the path to `Cargo.toml` is not specified, the `cargo update` command
    /// will be run in the directory with the `Cargo.lock` file.
    /// Leaving it blank will fix the entire workspace.
    pub fn new(
        cargo_lock: Lockfile,
        cargo_toml: Option<PathBuf>,
        path_to_cargo: Option<PathBuf>,
    ) -> Self {
        Self {
            lockfile: cargo_lock,
            manifest_path: cargo_toml,
            path_to_cargo,
        }
    }

    /// Returns a command that calls `cargo update` with the right arguments
    /// to attempt to fix this vulnerability.
    ///
    /// Note that the success of the command does not mean
    /// the vulnerability was actually fixed!
    /// It may remain if no semver-compatible fix was available.
    pub fn get_fix_command(&self, vulnerability: &Vulnerability, dry_run: bool) -> Command {
        let cargo_path: &Path = self.path_to_cargo.as_deref().unwrap_or(Path::new("cargo"));
        let pkg_name = &vulnerability.package.name;
        let mut command = Command::new(cargo_path);
        command.arg("update");
        if let Some(path) = self.manifest_path.as_ref() {
            command.arg("--manifest-path").arg(path);
        }
        if dry_run {
            command.arg("--dry-run");
        }
        // there can be more than one version of a given package in the lockfile, so we need to iterate over all of them
        for pkg in self.lockfile.packages.iter().filter(|pkg| {
            &pkg.name == pkg_name && vulnerability.versions.is_vulnerable(&pkg.version)
        }) {
            let pkgid = pkgid(pkg);
            command.arg(&pkgid);
        }

        command
    }
}

/// Returns a Cargo unique identifier for a package.
/// See `cargo help pkgid` for more info.
///
/// We need to pass these to `cargo update` because otherwise
/// the package specification will be ambiguous, and it will refuse to do anything.
fn pkgid(pkg: &Package) -> String {
    match pkg.source.as_ref() {
        Some(source) => format!("{}#{}@{}", source, pkg.name, pkg.version),
        None => format!("{}@{}", pkg.name, pkg.version),
    }
}
