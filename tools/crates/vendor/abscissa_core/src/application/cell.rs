//! Application cell: holder of application state.

use super::Application;
use once_cell::sync::OnceCell;
use std::ops::Deref;

/// Application cell: holder of application state.
pub struct AppCell<T>(OnceCell<T>);

impl<T> AppCell<T> {
    /// Create a new application cell.
    pub const fn new() -> AppCell<T> {
        Self(OnceCell::new())
    }
}

impl<A> AppCell<A>
where
    A: Application,
{
    /// Set the application state to the given value.
    ///
    /// This can only be performed once without causing a crash.
    pub(crate) fn set_once(&self, app: A) {
        self.0.set(app).unwrap_or_else(|_| {
            panic!("can't reset Abscissa application state (yet)!");
        })
    }
}

impl<A> Deref for AppCell<A>
where
    A: Application,
{
    type Target = A;

    #[allow(clippy::redundant_closure)]
    fn deref(&self) -> &A {
        self.0.get().unwrap_or_else(|| not_loaded())
    }
}

/// Error handler called if `get()` is invoked before the global
/// application state has been initialized.
///
/// This indicates a bug in the program accessing this type.
fn not_loaded() -> ! {
    panic!("Abscissa application state accessed before it has been initialized!")
}
