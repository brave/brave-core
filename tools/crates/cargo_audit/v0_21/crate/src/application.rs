//! Abscissa application for `cargo audit`
//!
//! <https://docs.rs/abscissa_core>

use std::sync::Arc;

use crate::{commands::CargoAuditCommand, config::AuditConfig};
use abscissa_core::{
    application::{self, AppCell},
    config::CfgCell,
    terminal::ColorChoice,
    trace, Application, FrameworkError, StandardPaths,
};

/// Application state
pub static APP: AppCell<CargoAuditApplication> = AppCell::new();

/// `cargo audit` application
#[derive(Debug)]
pub struct CargoAuditApplication {
    /// Application configuration.
    config: CfgCell<AuditConfig>,

    /// Application state.
    state: application::State<Self>,
}

/// Initialize a new application instance.
///
/// By default no configuration is loaded, and the framework state is
/// initialized to a default, empty state (no components, threads, etc).
impl Default for CargoAuditApplication {
    fn default() -> Self {
        Self {
            config: CfgCell::default(),
            state: application::State::default(),
        }
    }
}

impl Application for CargoAuditApplication {
    /// Entrypoint command for this application.
    type Cmd = CargoAuditCommand;

    /// Application configuration.
    type Cfg = AuditConfig;

    /// Paths to resources within the application.
    type Paths = StandardPaths;

    /// Accessor for application configuration.
    fn config(&self) -> Arc<AuditConfig> {
        self.config.read()
    }

    /// Borrow the application state immutably.
    fn state(&self) -> &application::State<Self> {
        &self.state
    }

    /// Register all components used by this application.
    fn register_components(&mut self, command: &Self::Cmd) -> Result<(), FrameworkError> {
        let components = self.framework_components(command)?;
        self.state.components_mut().register(components)
    }

    /// Post-configuration lifecycle callback.
    fn after_config(&mut self, config: Self::Cfg) -> Result<(), FrameworkError> {
        // Configure components
        self.state.components_mut().after_config(&config)?;
        self.config.set_once(config);
        Ok(())
    }

    /// Color configuration for this application.
    fn term_colors(&self, entrypoint: &CargoAuditCommand) -> ColorChoice {
        entrypoint.term_colors()
    }

    /// Get tracing configuration from command-line options
    fn tracing_config(&self, command: &CargoAuditCommand) -> trace::Config {
        if command.verbose {
            trace::Config::verbose()
        } else {
            trace::Config::default()
        }
    }
}
