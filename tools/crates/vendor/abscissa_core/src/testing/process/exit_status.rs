//! Exit statuses for terminated processes

use super::Guard;
use std::fmt;

/// Information about a process's exit status
pub struct ExitStatus<'cmd> {
    /// Exit code
    code: i32,

    /// Optional mutex guard ensuring exclusive access to this process.
    ///
    /// This is held by the exit status to ensure tests can inspect the results
    /// of executed commands with the lock held.
    #[allow(dead_code)]
    guard: Option<Guard<'cmd>>,
}

impl<'cmd> ExitStatus<'cmd> {
    /// Create a new exit status
    pub(super) fn new(code: i32, guard: Option<Guard<'cmd>>) -> Self {
        Self { code, guard }
    }

    /// Get the exit code
    pub fn code(&self) -> i32 {
        self.code
    }

    /// Did the process exit successfully?
    pub fn success(&self) -> bool {
        self.code == 0
    }

    /// Assert that the process exited successfully
    pub fn expect_success(&self) {
        assert_eq!(
            0, self.code,
            "process exited with error status: {}",
            self.code
        );
    }

    /// Assert that the process exited with the given code
    pub fn expect_code(&self, code: i32) {
        assert_eq!(
            code, self.code,
            "process exited with status code: {} (expected {})",
            self.code, code
        )
    }
}

impl<'cmd> fmt::Debug for ExitStatus<'cmd> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "ExitStatus({})", self.code)
    }
}
