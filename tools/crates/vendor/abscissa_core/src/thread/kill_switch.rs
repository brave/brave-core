//! Kill switches are a cooperative approach to requesting a thread terminate,
//! by setting a flag in one thread that another thread can periodically check
//! in order to determine if it should exit.

thread_local! {
    /// Boolean flag signaling to a thread to terminate
    static KILL_SWITCH: RefCell<Option<Arc<KillSwitch>>> = RefCell::new(None);
}

use std::{
    cell::RefCell,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
};

/// Thread kill switch.
///
/// This is a signal that the thread should terminate.
#[derive(Debug)]
pub(super) struct KillSwitch(AtomicBool);

impl KillSwitch {
    /// Create a new kill switch
    pub fn new() -> KillSwitch {
        KillSwitch(AtomicBool::new(false))
    }

    /// Throw the kill switch, indicating it's time to terminate
    pub fn throw(&self) {
        self.0.store(true, Ordering::Relaxed);
    }

    /// Has the kill switch been thrown?
    pub fn is_thrown(&self) -> bool {
        self.0.load(Ordering::Relaxed)
    }
}

/// Check whether the kill switch for this thread has been thrown.
///
/// Panics if no kill switch is configured for the current thread.
pub(super) fn is_thrown() -> bool {
    KILL_SWITCH.with(|ks| {
        ks.borrow()
            .as_ref()
            .expect("no kill switch configured for current thread")
            .is_thrown()
    })
}

/// Set the kill switch value for the current thread
pub(super) fn set(kill_switch: Arc<KillSwitch>) {
    KILL_SWITCH.with(|ks| *ks.borrow_mut() = Some(kill_switch));
}
