//! Abscissa tracing component

// TODO(tarcieri): logfile support?

use tracing_log::LogTracer;
use tracing_subscriber::{fmt::Formatter, reload::Handle, EnvFilter, FmtSubscriber};

use super::config::Config;
use crate::{terminal::ColorChoice, Component, FrameworkError, FrameworkErrorKind};

/// Abscissa component for initializing the `tracing` subsystem
#[derive(Component, Debug)]
#[component(core)]
pub struct Tracing {
    filter_handle: Handle<EnvFilter, Formatter>,
}

impl Tracing {
    /// Create a new [`Tracing`] component from the given [`Config`].
    pub fn new(config: Config, color_choice: ColorChoice) -> Result<Self, FrameworkError> {
        // Configure log/tracing interoperability by setting a `LogTracer` as
        // the global logger for the log crate, which converts all log events
        // into tracing events.
        LogTracer::init().map_err(|e| FrameworkErrorKind::ComponentError.context(e))?;

        // Construct a tracing subscriber with the supplied filter and enable reloading.
        let builder = FmtSubscriber::builder()
            .with_ansi(match color_choice {
                ColorChoice::Always => true,
                ColorChoice::AlwaysAnsi => true,
                ColorChoice::Auto => true,
                ColorChoice::Never => false,
            })
            .with_env_filter(config.filter)
            .with_filter_reloading();
        let filter_handle = builder.reload_handle();
        let subscriber = builder.finish();

        // Now set it as the global tracing subscriber and save the handle.
        tracing::subscriber::set_global_default(subscriber)
            .map_err(|e| FrameworkErrorKind::ComponentError.context(e))?;

        Ok(Self { filter_handle })
    }

    /// Return the currently-active tracing filter.
    pub fn filter(&self) -> String {
        self.filter_handle
            .with_current(|filter| filter.to_string())
            .expect("the subscriber is not dropped before the component is")
    }

    /// Reload the currently-active filter with the supplied value.
    ///
    /// This can be used to provide a dynamic tracing filter endpoint.
    pub fn reload_filter(&mut self, filter: impl Into<EnvFilter>) {
        self.filter_handle
            .reload(filter)
            .expect("the subscriber is not dropped before the component is");
    }
}
