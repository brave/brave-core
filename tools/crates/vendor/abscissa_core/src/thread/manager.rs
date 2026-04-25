//! Thread manager.

use super::{Name, Thread};
use crate::{FrameworkError, FrameworkErrorKind::ThreadError, Map};
use std::{convert::TryInto, sync};

/// Reader guard for the thread manager.
pub type Reader<'a> = sync::RwLockReadGuard<'a, Manager>;

/// Writer guard for the thread manager.
pub type Writer<'a> = sync::RwLockWriteGuard<'a, Manager>;

/// Thread manager that tracks threads spawned by the application and handles
/// shutting them down.
#[derive(Debug, Default)]
pub struct Manager {
    threads: Map<Name, Thread>,
}

impl Manager {
    /// Spawn a thread within the thread manager.
    pub fn spawn<F>(&mut self, name: impl TryInto<Name>, f: F) -> Result<(), FrameworkError>
    where
        F: FnOnce() + Send + 'static,
    {
        // TODO(tarcieri): propagate underlying error (after error handling refactor)
        let name = name
            .try_into()
            .ok()
            .ok_or_else(|| format_err!(ThreadError, "invalid thread name"))?;

        if self.threads.contains_key(&name) {
            fail!(ThreadError, "duplicate name: {}", name);
        }

        let thread = Thread::spawn(name.clone(), f)?;
        self.threads.insert(name, thread);

        Ok(())
    }

    /// Signal all running threads to terminate and then join them
    pub fn join(&mut self) -> Result<(), FrameworkError> {
        let mut names = Vec::with_capacity(self.threads.len());

        // Send termination request in advance prior to joining
        for (name, thread) in self.threads.iter() {
            names.push(name.clone());
            thread.request_termination();
        }

        // TODO(tarcieri): use `BTreeMap::into_values` when stable
        // See: <https://github.com/rust-lang/rust/issues/75294>
        for name in names.into_iter() {
            if let Some(thread) = self.threads.remove(&name) {
                thread.join()?;
            }
        }

        Ok(())
    }
}
