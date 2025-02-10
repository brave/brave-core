//! Subcommand runners which execute subprocesses

use super::{
    config::ConfigFile,
    process::{ExitStatus, Process},
};
use serde::Serialize;
use std::{
    ffi::OsString,
    io::{self, Write},
    process::{Command, Stdio},
    sync::{Arc, Mutex},
    time::Duration,
};
use termcolor::{BufferWriter, Color, ColorChoice, ColorSpec, WriteColor};

/// Name of the `cargo` subprocess
const CARGO_CMD: &str = "cargo";

/// Length of the default timeout (30 minutes)
const DEFAULT_TIMEOUT: Duration = Duration::from_secs(30 * 60);

/// Run a command via `cargo run`
#[derive(Clone, Debug)]
pub struct CmdRunner {
    /// Program to run
    program: OsString,

    /// Target binary name
    target_bin: Option<OsString>,

    /// Arguments to pass to the executable
    args: Vec<OsString>,

    /// Capture standard output to a pipe
    capture_stdout: bool,

    /// Capture standard error to a pipe
    capture_stderr: bool,

    /// Optional configuration file (deleted when no-longer used)
    config: Option<Arc<ConfigFile>>,

    /// Print invocation info
    print_info: bool,

    /// Optional mutex for serializing usages of this command
    mutex: Option<Arc<Mutex<()>>>,

    /// Timeout after which command should complete.
    timeout: Option<Duration>,
}

impl Default for CmdRunner {
    fn default() -> Self {
        let mut cmd = Self::new(CARGO_CMD);

        // Use a `Mutex` to ensure only copy of a command runs at a time
        cmd.exclusive();

        // Add `run` argument to `cargo`
        cmd.arg("run");
        cmd.arg("--");
        cmd
    }
}

impl CmdRunner {
    /// Run a target binary via cargo by passing `--bin` to `cargo run`.
    ///
    /// Use `CmdRunner::default()` if you only have one target binary in your
    /// Cargo workspace.
    pub fn target_bin<S>(bin: S) -> Self
    where
        S: Into<OsString>,
    {
        // Start with default settings
        let mut cmd = Self::default();

        // Set memoized target bin
        let bin = bin.into();
        cmd.target_bin = Some(bin.clone());

        // Clear arguments and replace them with ones for the given bin
        cmd.args.clear();
        cmd.arg("run");
        cmd.arg("--bin");
        cmd.arg(bin);
        cmd.arg("--");
        cmd
    }

    /// Create a new command runner which runs an arbitrary program at the
    /// given path.
    pub fn new<S>(program: S) -> Self
    where
        S: Into<OsString>,
    {
        Self {
            program: program.into(),
            target_bin: None,
            args: vec![],
            capture_stdout: false,
            capture_stderr: false,
            config: None,
            print_info: true,
            mutex: None,
            timeout: None,
        }
    }

    /// Append an argument to the set of arguments to run
    pub fn arg<S>(&mut self, arg: S) -> &mut Self
    where
        S: Into<OsString>,
    {
        self.args.push(arg.into());
        self
    }

    /// Append multiple arguments to the set of arguments to run
    pub fn args<I, S>(&mut self, args: I) -> &mut Self
    where
        I: IntoIterator<Item = S>,
        S: Into<OsString>,
    {
        self.args.extend(args.into_iter().map(|a| a.into()));
        self
    }

    /// Enable capturing of standard output
    pub fn capture_stdout(&mut self) -> &mut Self {
        self.capture_stdout = true;
        self
    }

    /// Enable capturing of standard error
    pub fn capture_stderr(&mut self) -> &mut Self {
        self.capture_stderr = true;
        self
    }

    /// Add the given configuration file
    pub fn config<C>(&mut self, config: &C) -> &mut Self
    where
        C: Serialize,
    {
        if self.config.is_some() {
            panic!("config file already added");
        }

        let target_bin = self
            .target_bin
            .as_ref()
            .cloned()
            .unwrap_or_else(|| "app".into());

        let config_file = ConfigFile::create(&target_bin, config);

        // Add `abscissa_core::EntryPoint`-compatible args to override config
        self.arg("-c");
        self.arg(config_file.path());

        self.config = Some(Arc::new(config_file));
        self
    }

    /// Serialize invocations of this command using a mutex
    pub fn exclusive(&mut self) -> &mut Self {
        if self.mutex.is_none() {
            self.mutex = Some(Arc::new(Mutex::new(())))
        }
        self
    }

    /// Disable printing a `+ run: ...` logline when running command
    pub fn quiet(&mut self) -> &mut Self {
        self.print_info = false;
        self
    }

    /// Set the timeout after which the command should complete.
    ///
    /// By default `CargoRunner` timeout will be used (30 minutes).
    pub fn timeout(&mut self, duration: Duration) -> &mut Self {
        self.timeout = Some(duration);
        self
    }

    /// Run the given subcommand
    pub fn run(&self) -> Process<'_> {
        let guard = self
            .mutex
            .as_ref()
            .map(|mutex| mutex.lock().expect("poisoned cmd mutex!"));

        if self.print_info {
            self.print_command().unwrap();
        }

        let stdout = if self.capture_stdout {
            Stdio::piped()
        } else {
            Stdio::inherit()
        };

        let stderr = if self.capture_stderr {
            Stdio::piped()
        } else {
            Stdio::inherit()
        };

        let child = Command::new(&self.program)
            .args(&self.args)
            .stdin(Stdio::piped())
            .stdout(stdout)
            .stderr(stderr)
            .spawn()
            .unwrap_or_else(|e| {
                panic!("error running command: {}", e);
            });

        Process::new(child, self.timeout.unwrap_or(DEFAULT_TIMEOUT), guard)
    }

    /// Get the exit status for the given subcommand
    pub fn status(&self) -> ExitStatus<'_> {
        self.run().wait().unwrap_or_else(|e| {
            panic!("error waiting for subprocess to terminate: {}", e);
        })
    }

    /// Print the command we're about to run
    fn print_command(&self) -> Result<(), io::Error> {
        let stdout = BufferWriter::stdout(ColorChoice::Auto);
        let mut buffer = stdout.buffer();

        buffer.set_color(ColorSpec::new().set_fg(Some(Color::Green)))?;
        write!(&mut buffer, "+ ")?;

        buffer.set_color(ColorSpec::new().set_fg(Some(Color::White)).set_bold(true))?;
        write!(&mut buffer, "run")?;

        buffer.reset()?;

        let cmd = self.program.to_string_lossy();
        let args: Vec<_> = self.args.iter().map(|arg| arg.to_string_lossy()).collect();
        writeln!(&mut buffer, ": {} {}", cmd, args.join(" "))?;

        stdout.print(&buffer)
    }
}
