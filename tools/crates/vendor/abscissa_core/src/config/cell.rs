//! Configuration cell: holder of application configuration.

use super::{Config, Reader};
use arc_swap::ArcSwapOption;
use std::sync::Arc;

/// Configuration cell: holder of application configuration.
#[derive(Debug, Default)]
pub struct CfgCell<C: Config> {
    /// Inner configuration state
    inner: ArcSwapOption<C>,
}

impl<C> CfgCell<C>
where
    C: Config,
{
    /// Set the application configuration to the given value.
    ///
    /// This can only be performed once without causing a crash.
    pub fn set_once(&self, config: C) {
        let old_config = self.inner.swap(Some(Arc::new(config)));

        if old_config.is_some() {
            panic!("can't reload Abscissa application config (yet)!");
        }
    }

    /// Read the current configuration.
    #[allow(clippy::redundant_closure)]
    pub fn read(&self) -> Reader<C> {
        self.inner.load_full().unwrap_or_else(|| not_loaded())
    }
}

/// Error handler called if `get()` is invoked before the global
/// application state has been initialized.
///
/// This indicates a bug in the program accessing this type.
fn not_loaded() -> ! {
    panic!("Abscissa application state accessed before it has been initialized!")
}
