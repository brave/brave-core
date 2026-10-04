//! Subprocesses spawned by runners

mod exit_status;
mod streams;

pub use self::{
    exit_status::ExitStatus,
    streams::{OutputStream, Stderr, Stdout},
};

use crate::{
    FrameworkError,
    FrameworkErrorKind::{ProcessError, TimeoutError},
};
use std::{
    io::{self, Write},
    process::{Child, ChildStdin},
    sync::MutexGuard,
    time::Duration,
};
use wait_timeout::ChildExt;

/// Mutex guard for a subprocess
pub(super) type Guard<'cmd> = MutexGuard<'cmd, ()>;

/// Subprocess under test spawned by `CargoRunner` or `CmdRunner`
#[derive(Debug)]
pub struct Process<'cmd> {
    /// Child process
    child: Child,

    /// Timeout after which process should complete
    timeout: Duration,

    /// Standard output (if captured)
    stdout: Option<Stdout>,

    /// Standard error (if captured)
    stderr: Option<Stderr>,

    /// Standard input
    stdin: ChildStdin,

    /// Optional mutex guard ensuring exclusive access to this process
    guard: Option<Guard<'cmd>>,
}

impl<'cmd> Process<'cmd> {
    /// Create a process from the given `Child`.
    ///
    /// This gets invoked from `CargoRunner::run`
    pub(super) fn new(mut child: Child, timeout: Duration, guard: Option<Guard<'cmd>>) -> Self {
        let stdout = child.stdout.take().map(Stdout::new);
        let stderr = child.stderr.take().map(Stderr::new);
        let stdin = child.stdin.take().unwrap();

        Self {
            child,
            timeout,
            stdout,
            stderr,
            stdin,
            guard,
        }
    }

    /// Gets a handle to the child's stdout.
    ///
    /// Panics if the child's stdout isn't captured (via `capture_stdout`)
    pub fn stdout(&mut self) -> &mut Stdout {
        self.stdout
            .as_mut()
            .expect("child stdout not captured (use 'capture_stdout' method)")
    }

    /// Gets a handle to the child's stderr.
    ///
    /// Panics if the child's stderr isn't captured (via `capture_stderr`)
    pub fn stderr(&mut self) -> &mut Stderr {
        self.stderr
            .as_mut()
            .expect("child stderr not captured (use 'capture_stderr' method)")
    }

    /// Wait for the child to exit
    pub fn wait(mut self) -> Result<ExitStatus<'cmd>, FrameworkError> {
        match self.child.wait_timeout(self.timeout)? {
            Some(status) => {
                let code = status.code().ok_or_else(|| {
                    format_err!(ProcessError, "no exit status returned from subprocess!")
                })?;

                Ok(ExitStatus::new(code, self.guard))
            }
            None => fail!(
                TimeoutError,
                "operation timed out after {} seconds",
                self.timeout.as_secs()
            ),
        }
    }
}

impl<'cmd> Write for Process<'cmd> {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.stdin.write(buf)
    }

    fn flush(&mut self) -> io::Result<()> {
        self.stdin.flush()
    }
}
