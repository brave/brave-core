//! Thread wrapper types.
//!
//! These types provide simple wrappers for Rust's core threading primitives.

mod kill_switch;
pub mod manager;
mod name;

pub use self::{manager::Manager, name::Name};

use self::kill_switch::KillSwitch;
use crate::{FrameworkError, FrameworkErrorKind::ThreadError};
use std::{io, sync::Arc, thread};

/// Join handles for Abscissa-managed threads.
pub type JoinHandle = thread::JoinHandle<()>;

/// Threads spawned and managed by Abscissa
#[derive(Debug)]
pub struct Thread {
    /// Name of the current thread
    name: Name,

    /// Kill switch used to terminate the thread
    kill_switch: Arc<KillSwitch>,

    /// Join handle to the thread
    handle: JoinHandle,
}

impl Thread {
    /// Spawn a new thread, executing the given runnable
    pub fn spawn<F>(name: Name, f: F) -> Result<Self, FrameworkError>
    where
        F: FnOnce() + Send + 'static,
    {
        let kill_switch = Arc::new(KillSwitch::new());
        let handle = spawn_thread(name.clone(), Arc::clone(&kill_switch), f)?;

        Ok(Self {
            name,
            kill_switch,
            handle,
        })
    }

    /// Get the name of this thread.
    pub fn name(&self) -> &Name {
        &self.name
    }

    /// Request that this thread terminate.
    ///
    /// Note this does not have immediate effect: it signals to the thread
    /// that it should exit, however the target thread needs to poll the
    /// `Thread::should_terminate()` flag in order to receive this signal
    /// (and exit accordingly when it is set).
    pub fn request_termination(&self) {
        self.kill_switch.throw();
    }

    /// Join to a running thread, waiting for it to finish
    pub fn join(self) -> Result<(), FrameworkError> {
        // Trigger the kill switch in order to signal the thread to stop.
        self.request_termination();

        // Wait for the other thread to exit
        self.handle
            .join()
            .map_err(|e| format_err!(ThreadError, "{:?}", e))?;

        Ok(())
    }
}

/// Check whether the currently running thread should exit, as signaled by
/// `Thread::request_termination()`.
///
/// Panics if called outside a thread spawned by `abscissa_core::Thread`.
pub fn should_terminate() -> bool {
    kill_switch::is_thrown()
}

/// Spawn a thread
fn spawn_thread<F>(name: Name, kill_switch: Arc<KillSwitch>, f: F) -> Result<JoinHandle, io::Error>
where
    F: FnOnce() + Send + 'static,
{
    // NOTE: `Name` ensures the absence of null bytes, which should prevent the
    // only condition under which this function could potentially panic.
    thread::Builder::new().name(name.into()).spawn(move || {
        kill_switch::set(kill_switch);
        f()
    })
}
