//! `cargo audit` subcommands

mod audit;

use self::audit::AuditCommand;
use crate::config::AuditConfig;
use abscissa_core::{config::Override, Command, Configurable, FrameworkError, Runnable};
use clap::Parser;
use std::{ops::Deref, path::PathBuf};

/// Name of the configuration file
///
/// This file allows setting some default auditing options.
pub const CONFIG_FILE: &str = "audit.toml";

/// `cargo audit` subcommands (presently only `audit`)
#[derive(Command, Debug, Parser, Runnable)]
#[command(bin_name = "cargo")]
pub enum CargoAuditSubCommand {
    /// The `cargo audit` subcommand
    #[command(about = "Audit Cargo.lock files for vulnerable crates")]
    Audit(AuditCommand),
}

/// `cargo audit` entrypoint
#[derive(Command, Debug, Parser)]
#[command(author, version, about)]
pub struct CargoAuditCommand {
    #[command(subcommand)]
    cmd: CargoAuditSubCommand,

    /// Increase verbosity setting
    #[arg(short = 'v', long, help = "Increase verbosity")]
    pub verbose: bool,
}

impl Runnable for CargoAuditCommand {
    fn run(&self) {
        self.cmd.run()
    }
}

impl Configurable<AuditConfig> for CargoAuditCommand {
    /// Location of `audit.toml` (if it exists)
    fn config_path(&self) -> Option<PathBuf> {
        // Check if the config file exists, and if it does not, ignore it.
        //
        // The order of precedence for which config file to use is:
        // 1. The current project's `.cargo` configuration directory.
        // 2. The current user's home directory configuration.

        let project_config_filename = PathBuf::from("./.cargo").join(CONFIG_FILE);
        if project_config_filename.exists() {
            return Some(project_config_filename);
        }

        let home_config_filename = home::cargo_home()
            .ok()
            .map(|cargo_home| cargo_home.join(CONFIG_FILE))?;

        if home_config_filename.exists() {
            Some(home_config_filename)
        } else {
            None
        }
    }

    /// Override loaded config with explicit command-line arguments
    fn process_config(&self, config: AuditConfig) -> Result<AuditConfig, FrameworkError> {
        match &self.cmd {
            CargoAuditSubCommand::Audit(cmd) => cmd.override_config(config),
        }
    }
}

impl Deref for CargoAuditCommand {
    type Target = AuditCommand;

    fn deref(&self) -> &AuditCommand {
        match &self.cmd {
            CargoAuditSubCommand::Audit(cmd) => cmd,
        }
    }
}
