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
        let cargo_path = self
            .path_to_cargo
            .as_deref()
            .unwrap_or_else(|| Path::new("cargo"));

        let pkg_name = &vulnerability.package.name;
        let mut command = Command::new(cargo_path);
        command.arg("update");
        if let Some(path) = self.manifest_path.as_ref() {
            command.arg("--manifest-path").arg(path);
        }
        if dry_run {
            command.arg("--dry-run");
        }
        // There can be more than one version of a given package in the
        // lockfile, so we need to iterate over all of them.
        //
        // Before we do, we need to add -- so that a lockfile entry starting
        // with -- isn't interpreted as a flag by cargo.
        command.arg("--");
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Advisory;

    #[test]
    fn fix_command_package_separator() {
        // Construct a lockfile with a name that could be interpreted as a flag.
        let package_name = "--config=/tmp/attacker-config";
        let lockfile: Lockfile = format!(
            r#"version = 3

[[package]]
name = "{package_name}"
version = "1.0.0"
"#
        )
        .parse()
        .expect("parse lock file");

        // Build an advisory and vulnerability with the same name.
        let advisory: Advisory = format!(
            r#"```toml
[advisory]
id = "RUSTSEC-2026-0001"
package = "{package_name}"
date = "2026-11-10"

[versions]
patched = [">=2.0.0"]
```

# Test advisory

Test.
"#
        )
        .parse()
        .expect("parse advisory");

        let vulnerability = Vulnerability::new(&advisory, &lockfile.packages[0]);

        // Build the fix command and validate that it looks as expected; that
        // is, with the -- before the package name.
        let args = Fixer::new(lockfile, Some("Cargo.toml".into()), None)
            .get_fix_command(&vulnerability, true)
            .get_args()
            .map(|arg| arg.to_str().expect("arg is valid str").to_owned())
            .collect::<Vec<_>>();

        assert_eq!(
            args,
            [
                "update",
                "--manifest-path",
                "Cargo.toml",
                "--dry-run",
                "--",
                "--config=/tmp/attacker-config@1.0.0",
            ]
        );
    }
}
