use crate::{Hashable, Level};
use alloc::string::{String, ToString};

/// A possibly-empty incremental Merkle frontier.
pub trait Frontier<H> {
    /// Appends a new value to the frontier at the next available slot.
    /// Returns true if successful and false if the frontier would exceed
    /// the maximum allowed depth.
    fn append(&mut self, value: H) -> bool;

    /// Obtains the current root of this Merkle frontier by hashing
    /// against empty nodes up to the maximum height of the pruned
    /// tree that the frontier represents.
    fn root(&self) -> H;
}

impl Hashable for String {
    fn empty_leaf() -> Self {
        "_".to_string()
    }

    fn combine(_: Level, a: &Self, b: &Self) -> Self {
        a.to_string() + b
    }
}

impl<H: Hashable> Hashable for Option<H> {
    fn empty_leaf() -> Self {
        Some(H::empty_leaf())
    }

    fn combine(l: Level, a: &Self, b: &Self) -> Self {
        match (a, b) {
            (Some(a), Some(b)) => Some(H::combine(l, a, b)),
            _ => None,
        }
    }
}
