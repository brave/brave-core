//! The `[[patch]]` section

use crate::dependency::Dependency;
use serde::{Deserialize, Serialize};

/// The `[[patch]]` section of `Cargo.lock`
#[derive(Clone, Debug, Default, Deserialize, Eq, PartialEq, Serialize)]
pub struct Patch {
    /// Unused patches
    pub unused: Vec<Dependency>,
}

impl Patch {
    /// Is the `[patch]` section empty?
    pub fn is_empty(&self) -> bool {
        self.unused.is_empty()
    }
}
