//! The `cargo audit` subcommand

#[cfg(feature = "fix")]
mod fix;

#[cfg(feature = "binary-scanning")]
mod binary_scanning;

use crate::{
    auditor::Auditor,
    cli_config::CliConfig,
    config::{AuditConfig, DenyOption},
    error::display_err_with_source,
    lockfile,
    prelude::*,
};
use abscissa_core::{config::Override, terminal::ColorChoice, FrameworkError};
use clap::Parser;
use rustsec::platforms::target::{Arch, OS};
use std::{path::PathBuf, process::exit};

#[cfg(feature = "binary-scanning")]
use self::binary_scanning::BinCommand;
#[cfg(feature = "fix")]
use self::fix::FixCommand;
#[cfg(any(feature = "fix", feature = "binary-scanning"))]
use clap::Subcommand;

/// The `cargo audit` subcommand
#[derive(Command, Clone, Default, Debug, Parser)]
#[command(version)]
pub struct AuditCommand {
    /// Optional subcommand (used for `cargo audit fix` and `cargo audit bin`)
    #[cfg(any(feature = "fix", feature = "binary-scanning"))]
    #[command(subcommand)]
    subcommand: Option<AuditSubcommand>,

    /// Colored output configuration
    #[arg(
        short = 'c',
        long = "color",
        help = "color configuration: always, never (default: auto)"
    )]
    color: Option<String>,

    /// Filesystem path to the advisory database git repository
    #[arg(
        short,
        long = "db",
        help = "advisory database git repo path (default: ~/.cargo/advisory-db)"
    )]
    db: Option<PathBuf>,

    /// Deny flag
    #[arg(
        short = 'D',
        long = "deny",
        help = "exit with an error on: warnings (any), unmaintained, unsound, yanked"
    )]
    deny: Vec<DenyOption>,

    /// Path to `Cargo.lock`
    #[arg(
        short = 'f',
        long = "file",
        help = "Cargo lockfile to inspect (or `-` for STDIN, default: Cargo.lock)"
    )]
    file: Option<PathBuf>,

    /// Advisory IDs to ignore
    #[arg(
        long = "ignore",
        value_name = "ADVISORY_ID",
        help = "Advisory id to ignore (can be specified multiple times)"
    )]
    ignore: Vec<String>,

    /// Ignore the sources of packages in Cargo.toml
    #[arg(
        long = "ignore-source",
        help = "Ignore sources of packages in Cargo.toml, matching advisories regardless of source"
    )]
    ignore_source: bool,

    /// Skip fetching the advisory database git repository
    #[arg(
        short = 'n',
        long = "no-fetch",
        help = "do not perform a git fetch on the advisory DB"
    )]
    no_fetch: bool,

    /// Allow stale advisory databases that haven't been recently updated
    #[arg(long = "stale", help = "allow stale database")]
    stale: bool,

    /// Target CPU architecture to find vulnerabilities for
    #[arg(
        long = "target-arch",
        help = "filter vulnerabilities by CPU (default: no filter)"
    )]
    target_arch: Option<Arch>,

    /// Target OS to find vulnerabilities for
    #[arg(
        long = "target-os",
        help = "filter vulnerabilities by OS (default: no filter)"
    )]
    target_os: Option<OS>,

    /// URL to the advisory database git repository
    #[arg(short = 'u', long = "url", help = "URL for advisory database git repo")]
    url: Option<String>,

    /// Quiet mode - avoids printing extraneous information
    #[arg(
        short = 'q',
        long = "quiet",
        help = "Avoid printing unnecessary information"
    )]
    quiet: bool,

    /// Output reports as JSON
    #[arg(long = "json", help = "Output report in JSON format")]
    output_json: bool,
}

/// Subcommands of `cargo audit`
#[cfg(any(feature = "fix", feature = "binary-scanning"))]
#[derive(Subcommand, Clone, Debug, Runnable)]
pub enum AuditSubcommand {
    /// `cargo audit fix` subcommand
    #[cfg(feature = "fix")]
    #[command(about = "automatically upgrade vulnerable dependencies")]
    Fix(FixCommand),

    /// `cargo audit bin` subcommand
    #[cfg(feature = "binary-scanning")]
    #[command(
        about = "scan compiled binaries",
        long_about = "Scan compiled binaries for known vulnerabilities.

Performs a complete scan if the binary is built with 'cargo auditable'.
If not, recovers a part of the dependency list from panic messages."
    )]
    Bin(BinCommand),
}

impl AuditCommand {
    /// Get the color configuration
    pub fn color_config(&self) -> Option<ColorChoice> {
        // suppress the warning that occurs with the `binary-scanning` feature disabled
        #[allow(unused_mut)]
        let mut raw_color_setting = self.color.as_ref();
        #[cfg(feature = "binary-scanning")]
        if let Some(AuditSubcommand::Bin(ref command)) = self.subcommand {
            raw_color_setting = command.color.as_ref()
        };
        raw_color_setting.map(|colors| match colors.as_ref() {
            "always" => ColorChoice::Always,
            "auto" => ColorChoice::Auto,
            "never" => ColorChoice::Never,
            _ => panic!("invalid color choice setting: {}", &colors),
        })
    }
}

impl From<AuditCommand> for CliConfig {
    fn from(c: AuditCommand) -> Self {
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

impl Override<AuditConfig> for AuditCommand {
    fn override_config(&self, config: AuditConfig) -> Result<AuditConfig, FrameworkError> {
        #[cfg(feature = "binary-scanning")]
        if let Some(AuditSubcommand::Bin(bin)) = &self.subcommand {
            return CliConfig::from(bin.clone()).override_config(config);
        }
        CliConfig::from(self.clone()).override_config(config)
    }
}

impl Runnable for AuditCommand {
    fn run(&self) {
        #[cfg(feature = "fix")]
        if let Some(AuditSubcommand::Fix(fix)) = &self.subcommand {
            fix.run();
            exit(0)
        }

        #[cfg(feature = "binary-scanning")]
        if let Some(AuditSubcommand::Bin(bin)) = &self.subcommand {
            bin.run();
            exit(0)
        }

        let maybe_path = self.file.as_deref();
        // It is important to generate the lockfile before initializing the auditor,
        // otherwise we might deadlock because both need the Cargo package lock
        let path = lockfile::locate_or_generate(maybe_path).unwrap_or_else(|e| {
            status_err!("{}", display_err_with_source(&e));
            exit(2);
        });
        let mut auditor = self.auditor();
        let report = auditor.audit_lockfile(&path);
        match report {
            Ok(report) => {
                if auditor.should_exit_with_failure(&report) {
                    exit(1);
                }
                exit(0);
            }
            Err(e) => {
                status_err!("{}", display_err_with_source(&e));
                exit(2);
            }
        };
    }
}

impl AuditCommand {
    /// Initialize `Auditor`
    pub fn auditor(&self) -> Auditor {
        Auditor::new(&APP.config())
    }
}
