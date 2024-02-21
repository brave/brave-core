//! The representation of config as it appears in raw CLI arguments

use std::path::PathBuf;

use abscissa_core::error::context::Context;
use abscissa_core::error::framework::FrameworkErrorKind;
use abscissa_core::FrameworkError;
use rustsec::platforms::target::{Arch, OS};

use crate::config::{AuditConfig, DenyOption, OutputFormat};

#[derive(Debug, Clone)]
pub struct CliConfig {
    /// Filesystem path to the advisory database git repository
    pub db: Option<PathBuf>,

    /// Deny flag
    pub deny: Vec<DenyOption>,

    /// Advisory IDs to ignore
    pub ignore: Vec<String>,

    /// Ignore the sources of packages in Cargo.toml
    pub ignore_source: bool,

    /// Skip fetching the advisory database git repository
    pub no_fetch: bool,

    /// Allow stale advisory databases that haven't been recently updated
    pub stale: bool,

    /// Target CPU architecture to find vulnerabilities for
    pub target_arch: Option<Arch>,

    /// Target OS to find vulnerabilities for
    pub target_os: Option<OS>,

    /// URL to the advisory database git repository
    pub url: Option<String>,

    /// Quiet mode - avoids printing extraneous information
    pub quiet: bool,

    /// Output reports as JSON
    pub output_json: bool,
}

// we cannot `impl Override<AuditConfig>` because this struct does not implement `abscissa::Command`
impl CliConfig {
    pub fn override_config(&self, mut config: AuditConfig) -> Result<AuditConfig, FrameworkError> {
        if let Some(db) = &self.db {
            config.database.path = Some(db.into());
        }

        for advisory_id in &self.ignore {
            config.advisories.ignore.push(
                advisory_id
                    .parse()
                    .map_err(|e| Context::new(FrameworkErrorKind::ParseError, Some(Box::new(e))))?,
            );
        }

        config.advisories.ignore_source |= self.ignore_source;
        config.database.fetch |= !self.no_fetch;
        config.database.stale |= self.stale;

        if let Some(target_arch) = self.target_arch {
            config.target.arch = Some(target_arch);
        }

        if let Some(target_os) = self.target_os {
            config.target.os = Some(target_os);
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

        if self.output_json {
            config.output.format = OutputFormat::Json;
        }

        Ok(config)
    }
}
