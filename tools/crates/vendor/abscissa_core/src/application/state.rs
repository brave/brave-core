//! Application state managed by the framework.

use crate::{application::Application, component, thread};
use std::sync::RwLock;

/// Error message to use for mutex error panics.
const MUTEX_ERR_MSG: &str = "error acquiring mutex";

/// Framework-managed application state
#[derive(Debug, Default)]
pub struct State<A: Application + 'static> {
    /// Application components.
    components: RwLock<component::Registry<A>>,

    /// Application paths.
    paths: A::Paths,

    /// Thread manager.
    threads: RwLock<thread::Manager>,
}

impl<A> State<A>
where
    A: Application + 'static,
{
    /// Obtain a read-only lock on the component registry.
    pub fn components(&self) -> component::registry::Reader<'_, A> {
        self.components.read().expect(MUTEX_ERR_MSG)
    }

    /// Obtain a mutable lock on the component registry.
    pub fn components_mut(&self) -> component::registry::Writer<'_, A> {
        self.components.write().expect(MUTEX_ERR_MSG)
    }

    /// Borrow the application paths.
    pub fn paths(&self) -> &A::Paths {
        &self.paths
    }

    /// Obtain a read-only lock on the thread manager.
    pub fn threads(&self) -> thread::manager::Reader<'_> {
        self.threads.read().expect(MUTEX_ERR_MSG)
    }

    /// Obtain a mutable lock on the component registry.
    pub fn threads_mut(&self) -> thread::manager::Writer<'_> {
        self.threads.write().expect(MUTEX_ERR_MSG)
    }
}
