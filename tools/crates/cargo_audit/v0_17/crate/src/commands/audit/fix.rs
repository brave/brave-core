//! The `cargo audit fix` subcommand

use crate::{auditor::Auditor, lockfile, prelude::*};
use abscissa_core::{Command, Runnable};
use clap::Parser;
use rustsec::Fixer;
use std::{
    path::{Path, PathBuf},
    process::exit,
};

#[derive(Command, Clone, Default, Debug, Parser)]
#[clap(author, version, about)]
pub struct FixCommand {
    /// Path to `Cargo.lock`
    #[clap(short = 'f', long = "file", help = "Cargo lockfile to inspect")]
    file: Option<PathBuf>,

    /// Perform a dry run
    #[clap(long = "dry-run", help = "perform a dry run for the fix")]
    dry_run: bool,
}

impl FixCommand {
    /// Initialize `Auditor`
    pub fn auditor(&self) -> Auditor {
        Auditor::new(&APP.config())
    }

    /// Locate `Cargo.toml`
    // TODO(tarcieri): ability to specify path
    pub fn cargo_toml_path(&self) -> PathBuf {
        PathBuf::from("Cargo.toml")
    }

    /// Locate `Cargo.lock`
    pub fn cargo_lock_path(&self) -> Option<&Path> {
        self.file.as_deref()
    }
}

impl Runnable for FixCommand {
    fn run(&self) {
        let report = self.auditor().audit_lockfile(self.cargo_lock_path());

        let report = match report {
            Ok(report) => {
                if report.vulnerabilities.list.is_empty() {
                    exit(0);
                }
                report
            }
            Err(e) => {
                status_err!("{}", e);
                exit(2);
            }
        };

        let mut fixer = Fixer::new(self.cargo_toml_path()).unwrap_or_else(|e| {
            status_err!(
                "couldn't load manifest from {}: {}",
                self.cargo_toml_path().display(),
                e
            );
            exit(1);
        });

        let dry_run = self.dry_run;
        let dry_run_info = if dry_run { " (dry run)" } else { "" };

        status_ok!(
            "Fixing",
            "vulnerable dependencies in `{}`{}",
            self.cargo_toml_path().display(),
            dry_run_info
        );

        for vulnerability in &report.vulnerabilities.list {
            if let Err(e) = fixer.fix(vulnerability, dry_run) {
                status_warn!("{}", e);
            }
        }

        if let Err(e) = lockfile::generate() {
            status_err!("{}", e);
            exit(2);
        }
    }
}
