//! Component handles: opaque references to registered components

use super::{id::Id, registry::Index};
use std::fmt;

/// Component handles are references to components which have been registered
/// with a `component::Registry`.
///
/// However, unlike normal Rust references, component handles are a "weak"
/// reference which is not checked by the borrow checker. This allows for
/// complex reference graphs which are otherwise inexpressible in Rust.
#[derive(Copy, Clone, Eq, PartialEq, Ord, PartialOrd)]
pub struct Handle {
    /// Component name
    name: Id,

    /// Registry index
    pub(super) index: Index,
}

impl Handle {
    /// Create a new handle from a component's name and index
    pub(crate) fn new(name: Id, index: Index) -> Self {
        Self { name, index }
    }

    /// Get the identifier of the component this handle points to
    pub fn id(self) -> Id {
        self.name
    }
}

impl fmt::Debug for Handle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Handle({})", self.id().as_ref())
    }
}
