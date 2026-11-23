use std::path::PathBuf;

use clap::Parser;

use crate::{auditor::update_db, config::AuditConfig};

#[derive(Clone, Default, Debug, Parser)]
#[command(author, version, about)]
pub(crate) struct UpdateDbCommand {
    /// Filesystem path to the advisory database git repository
    #[arg(
        short,
        long = "db",
        help = "advisory database git repo path (default: ~/.cargo/advisory-db)"
    )]
    db: Option<PathBuf>,
    /// URL to the advisory database git repository
    #[arg(short = 'u', long = "url", help = "URL for advisory database git repo")]
    url: Option<String>,
}

impl UpdateDbCommand {
    pub(crate) fn run(&self, config: &AuditConfig) {
        let mut config = config.clone();
        if let Some(db) = &self.db {
            config.database.path = Some(db.clone());
        }

        if let Some(url) = &self.url {
            config.database.url = Some(url.clone());
        }

        let path = config.database.path();
        update_db(&path, &config.database, config.output.is_quiet());
    }
}
