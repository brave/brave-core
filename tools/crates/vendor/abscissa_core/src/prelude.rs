//! Core prelude: imported in every application's `prelude.rs`

/// Commonly used Abscissa traits
pub use crate::{Application, Command, Runnable};

/// Error macros
pub use crate::{ensure, fail, fatal, format_err};

/// Tracing macros
pub use crate::tracing::{debug, error, event, info, span, trace, warn, Level};

/// Status macros
pub use crate::{status_err, status_info, status_ok, status_warn};
