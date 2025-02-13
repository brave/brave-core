//! Configuration loader

use super::Config;
use crate::FrameworkError;
use std::path::PathBuf;

/// Command type with which a configuration file is associated
pub trait Configurable<Cfg: Config> {
    /// Path to the command's configuration file. Returns an error by default.
    fn config_path(&self) -> Option<PathBuf> {
        None
    }

    /// Process the configuration after it has been loaded, potentially
    /// modifying it or returning an error if options are incompatible
    fn process_config(&self, config: Cfg) -> Result<Cfg, FrameworkError> {
        Ok(config)
    }
}
