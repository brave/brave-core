//! The `cargo audit bin` subcommand

use crate::{auditor::Auditor, prelude::*};
use clap::Parser;
use std::{path::PathBuf, process::exit};

#[cfg(feature = "binary-scanning")]
/// The `cargo audit` subcommand
#[derive(Command, Clone, Default, Debug, Parser)]
#[command()]
pub struct BinCommand {
    /// Paths to the binaries to be scanned
    #[arg(
        value_parser,
        required = true,
        help = "Paths to the binaries to be scanned"
    )]
    binary_paths: Vec<PathBuf>,
}

impl Runnable for BinCommand {
    fn run(&self) {
        let report = self.auditor().audit_binaries(&self.binary_paths);
        if report.vulnerabilities_found {
            exit(1)
        } else if report.errors_encountered {
            exit(2)
        } else {
            exit(0)
        }
    }
}

impl BinCommand {
    /// Initialize `Auditor`
    pub fn auditor(&self) -> Auditor {
        Auditor::new(&APP.config())
    }
}
