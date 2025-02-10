//! Override values in the configuration file with command-line options

use crate::{Command, Config, FrameworkError};

/// Use options from the given `Command` to override settings in the config.
pub trait Override<Cfg: Config>: Command {
    /// Process the given command line options, overriding settings from
    /// a configuration file using explicit flags taken from command-line
    /// arguments.
    ///
    /// This provides a canonical way to interpret global configuration
    /// settings when dealing with both a config file and options passed
    /// on the command line, and a unified way of accessing this information
    /// from components or in the application: from the global config.
    fn override_config(&self, config: Cfg) -> Result<Cfg, FrameworkError> {
        Ok(config)
    }
}
