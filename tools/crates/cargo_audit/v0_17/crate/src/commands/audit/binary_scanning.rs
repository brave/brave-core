//! The `cargo audit bin` subcommand

use crate::{
    auditor::Auditor,
    cli_config::CliConfig,
    config::{AuditConfig, DenyOption},
    prelude::*,
};
use abscissa_core::{config::Override, FrameworkError};
use clap::Parser;
use rustsec::platforms::target::{Arch, OS};
use std::{path::PathBuf, process::exit};

#[cfg(feature = "binary-scanning")]
/// The `cargo audit` subcommand
#[derive(Command, Clone, Default, Debug, Parser)]
pub struct BinCommand {
    /// Get help information
    #[clap(short = 'h', long = "help", help = "output help information and exit")]
    help: bool,

    /// Colored output configuration
    #[clap(
        short = 'c',
        long = "color",
        help = "color configuration: always, never (default: auto)"
    )]
    pub color: Option<String>,

    /// Filesystem path to the advisory database git repository
    #[clap(
        short,
        long = "db",
        help = "advisory database git repo path (default: ~/.cargo/advisory-db)"
    )]
    db: Option<PathBuf>,

    /// Deny flag
    #[clap(
        short = 'D',
        long = "deny",
        help = "exit with an error on: warnings (any), unmaintained, unsound, yanked"
    )]
    deny: Vec<DenyOption>,

    /// Advisory IDs to ignore
    #[clap(
        long = "ignore",
        value_name = "ADVISORY_ID",
        help = "Advisory id to ignore (can be specified multiple times)"
    )]
    ignore: Vec<String>,

    /// Ignore the sources of packages in the audit data
    #[clap(
        long = "ignore-source",
        help = "Ignore sources of packages in the audit data, matching advisories regardless of source"
    )]
    ignore_source: bool,

    /// Skip fetching the advisory database git repository
    #[clap(
        short = 'n',
        long = "no-fetch",
        help = "do not perform a git fetch on the advisory DB"
    )]
    no_fetch: bool,

    /// Allow stale advisory databases that haven't been recently updated
    #[clap(long = "stale", help = "allow stale database")]
    stale: bool,

    /// Target CPU architecture to find vulnerabilities for
    #[clap(
        long = "target-arch",
        help = "filter vulnerabilities by CPU (default: no filter)"
    )]
    target_arch: Option<Arch>,

    /// Target OS to find vulnerabilities for
    #[clap(
        long = "target-os",
        help = "filter vulnerabilities by OS (default: no filter)"
    )]
    target_os: Option<OS>,

    /// URL to the advisory database git repository
    #[clap(short = 'u', long = "url", help = "URL for advisory database git repo")]
    url: Option<String>,

    /// Quiet mode - avoids printing extraneous information
    #[clap(
        short = 'q',
        long = "quiet",
        help = "Avoid printing unnecessary information"
    )]
    quiet: bool,

    /// Output reports as JSON
    #[clap(long = "json", help = "Output report in JSON format")]
    output_json: bool,

    /// Paths to the binaries to be scanned
    #[clap(
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

impl From<BinCommand> for CliConfig {
    fn from(c: BinCommand) -> Self {
        CliConfig {
            db: c.db,
            deny: c.deny,
            ignore: c.ignore,
            ignore_source: c.ignore_source,
            no_fetch: c.no_fetch,
            stale: c.stale,
            target_arch: c.target_arch,
            target_os: c.target_os,
            url: c.url,
            quiet: c.quiet,
            output_json: c.output_json,
        }
    }
}

impl Override<AuditConfig> for BinCommand {
    fn override_config(&self, config: AuditConfig) -> Result<AuditConfig, FrameworkError> {
        CliConfig::from(self.clone()).override_config(config)
    }
}

impl BinCommand {
    /// Initialize `Auditor`
    pub fn auditor(&self) -> Auditor {
        Auditor::new(&APP.config())
    }
}
