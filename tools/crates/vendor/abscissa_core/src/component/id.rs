//! Component identifiers
//!
//! By convention these are Rust paths to the component types
// TODO(tarcieri): enforce this convention via e.g. custom derive?

use std::fmt;

/// Identifier for an individual component
///
/// This should ideally match the Rust path name to the corresponding type.
// TODO(tarcieri): obtain this automatically via `std::module_path`?
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, Ord, PartialOrd)]
pub struct Id(&'static str);

impl Id {
    /// Create a new component identifier
    // TODO(tarcieri): make this method private in the future
    pub const fn new(id: &'static str) -> Id {
        Id(id)
    }
}

impl AsRef<str> for Id {
    fn as_ref(&self) -> &str {
        self.0
    }
}

impl fmt::Display for Id {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}
