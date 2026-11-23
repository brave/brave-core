//! The `cargo audit` subcommand

use std::{
    fmt,
    io::{self, IsTerminal},
    path::PathBuf,
    process::exit,
};

use clap::{Parser, Subcommand, ValueEnum};

use crate::{
    auditor::Auditor,
    config::{AuditConfig, DenyOption, FilterList, OutputFormat},
    error::{Error, ErrorKind, display_err_with_source},
    lockfile,
    prelude::*,
    terminal::ColorChoice,
};

#[cfg(feature = "fix")]
mod fix;
#[cfg(feature = "fix")]
use fix::FixCommand;

#[cfg(feature = "binary-scanning")]
mod binary_scanning;
#[cfg(feature = "binary-scanning")]
use binary_scanning::BinCommand;

mod update_db;
use update_db::UpdateDbCommand;

#[derive(Debug, Clone, Copy, Default, ValueEnum)]
#[value(rename_all = "kebab-case")] // If you change this, remember to update `fmt::Display` impl.
enum Color {
    Always,
    // TODO: Should this be also supported?
    // AlwaysAnsi,
    #[default]
    Auto,
    Never,
}

impl From<Color> for ColorChoice {
    fn from(value: Color) -> Self {
        match value {
            Color::Always => Self::Always,
            Color::Auto => Self::Auto,
            Color::Never => Self::Never,
        }
    }
}

impl fmt::Display for Color {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // NOTE: This must be in sync with values genereted in ValueEnum implementation.
        match self {
            Self::Always => f.write_str("always"),
            Self::Auto => f.write_str("auto"),
            Self::Never => f.write_str("never"),
        }
    }
}

/// The `cargo audit` subcommand
#[derive(Clone, Default, Debug, Parser)]
#[command(version)]
pub struct AuditCommand {
    /// Optional subcommand (used for `cargo audit fix` and `cargo audit bin`)
    #[command(subcommand)]
    subcommand: Option<AuditSubcommand>,

    /// Colored output configuration
    #[arg(
        short = 'c',
        long = "color",
        help = "color configuration (default: auto)"
    )]
    color: Option<Color>,

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

    /// Skip checking for yanked crates
    #[arg(long = "no-yanked", help = "do not check for yanked crates")]
    no_yanked: bool,

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
        help = "filter vulnerabilities by CPU (default: no filter). Can be specified multiple times"
    )]
    target_arch: Vec<String>,

    /// Target OS to find vulnerabilities for
    #[arg(
        long = "target-os",
        help = "filter vulnerabilities by OS (default: no filter). Can be specified multiple times"
    )]
    target_os: Vec<String>,

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

    /// Output format
    #[arg(
        long = "format",
        value_name = "FORMAT",
        help = "Output format: terminal, json, or sarif"
    )]
    output_format: Option<OutputFormat>,

    /// Output reports as JSON
    #[arg(long = "json", help = "Output report in JSON format")]
    output_json: bool,
}

/// Subcommands of `cargo audit`
#[derive(Subcommand, Clone, Debug)]
pub(super) enum AuditSubcommand {
    /// `cargo audit fix` subcommand
    #[cfg(feature = "fix")]
    #[command(about = "automatically upgrade vulnerable dependencies")]
    Fix(FixCommand),

    /// `cargo audit update-db` subcommand
    #[command(about = "update the advisory database")]
    UpdateDb(UpdateDbCommand),

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
    pub(crate) fn term_colors(&self) -> ColorChoice {
        if let Some(color) = self.color {
            return color.into();
        }

        match std::env::var("CARGO_TERM_COLOR") {
            Ok(e) if e == "always" => ColorChoice::Always,
            Ok(e) if e == "never" => ColorChoice::Never,
            Ok(e) if e == "auto" => ColorChoice::Auto,
            _ => match io::stdout().is_terminal() {
                true => ColorChoice::Auto,
                false => ColorChoice::Never,
            },
        }
    }

    /// Override loaded config with explicit command-line arguments
    pub fn override_config(&self, config: AuditConfig) -> Result<AuditConfig, Error> {
        let mut config = config;
        if let Some(db) = &self.db {
            config.database.path = Some(db.into());
        }

        for advisory_id in &self.ignore {
            config.advisories.ignore.push(
                advisory_id
                    .parse()
                    .map_err(|e| ErrorKind::Parse.context(e))?,
            );
        }

        config.database.fetch &= !self.no_fetch;
        config.database.stale |= self.stale;

        if !self.target_arch.is_empty() {
            config.target.arch = Some(FilterList::Many(self.target_arch.clone()));
        }

        if !self.target_os.is_empty() {
            config.target.os = Some(FilterList::Many(self.target_os.clone()));
        }

        if let Some(url) = &self.url {
            config.database.url = Some(url.clone())
        }

        for kind in &self.deny {
            if *kind == DenyOption::Warnings {
                config.output.deny = DenyOption::all();
            } else {
                config.output.deny.push(*kind);
            }
        }

        config.output.quiet |= self.quiet;
        if self.quiet {
            config.output.show_tree = false;
        }

        // Handle output format (--json flag takes precedence for backward compatibility)
        if self.output_json {
            config.output.format = OutputFormat::Json;
        } else if let Some(format) = self.output_format {
            config.output.format = format;
        }

        if self.no_yanked {
            config.yanked.enabled = false;
        }

        Ok(config)
    }

    /// Run the `cargo audit` subcommand with the given configuration
    pub fn run(&self, config: &AuditConfig) {
        match &self.subcommand {
            #[cfg(feature = "binary-scanning")]
            Some(AuditSubcommand::Bin(bin)) => {
                bin.run(config);
                exit(0);
            }
            #[cfg(feature = "fix")]
            Some(AuditSubcommand::Fix(fix)) => {
                fix.run(config);
                exit(0);
            }
            Some(AuditSubcommand::UpdateDb(update_db)) => {
                update_db.run(config);
                exit(0);
            }
            None => {}
        }

        let maybe_path = self.file.as_deref();
        // It is important to generate the lockfile before initializing the auditor,
        // otherwise we might deadlock because both need the Cargo package lock
        let path = lockfile::locate_or_generate(maybe_path).unwrap_or_else(|e| {
            status_err!("{}", display_err_with_source(&e));
            exit(2);
        });
        let mut auditor = Auditor::new(config);
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

#[cfg(test)]
mod tests {
    use super::*;

    /// Ensure that when the database fetch option is false in the config file
    /// that it takes precedence when the CLI --no-fetch flag is _not_ set.
    #[test]
    fn override_default_fetch_option() {
        // Assert the default value for the fetch option is true
        let mut config: AuditConfig = AuditConfig::default();
        assert!(config.database.fetch);

        let mut audit_command = AuditCommand::default();

        let overridden_config = audit_command.override_config(config.clone()).unwrap();
        assert!(overridden_config.database.fetch);

        // as the CLI flag --no-fetch is false when not provided
        // override_config should not change the fetch option
        // when it is set to false in the config file
        config.database.fetch = false;
        let overridden_config = audit_command.override_config(config.clone()).unwrap();
        assert!(!overridden_config.database.fetch);

        config.database.fetch = true;
        audit_command.no_fetch = true;
        let overridden_config = audit_command.override_config(config.clone()).unwrap();
        assert!(!overridden_config.database.fetch);
    }
}
