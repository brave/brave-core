//! Terminal handling (colors, status messages)
//!
//! Presently provides a Cargo-like visual style.

use std::{
    io::{self, Write},
    sync::OnceLock,
};

pub(crate) use termcolor::{Color, ColorChoice};
use termcolor::{ColorSpec, StandardStream, WriteColor};

/// Terminal streams
static STREAMS: OnceLock<Streams> = OnceLock::new();

/// Initialize the terminal subsystem, registering the [`Streams`] static
pub(crate) fn init(color_choice: ColorChoice) {
    STREAMS
        .set(Streams {
            stdout: StandardStream::stdout(color_choice),
            stderr: StandardStream::stderr(color_choice),
        })
        .unwrap_or_else(|_| panic!("terminal streams already initialized!"));
}

/// Get the terminal [`Streams`]
fn streams() -> &'static Streams {
    STREAMS
        .get()
        .expect("terminal streams not yet initialized!")
}

/// Terminal streams
struct Streams {
    /// Standard output
    stdout: StandardStream,

    /// Standard error
    stderr: StandardStream,
}

/// Print a success status message (in green if colors are enabled)
macro_rules! status_ok {
    ($status:expr, $msg:expr) => {
        $crate::terminal::Status::new()
            .justified()
            .bold()
            .color($crate::terminal::Color::Green)
            .status($status)
            .print_stderr($msg)
            .unwrap();
    };
    ($status:expr, $fmt:expr, $($arg:tt)+) => {
        $crate::terminal::status_ok!($status, format!($fmt, $($arg)+));
    };
}

pub(crate) use status_ok;

/// Print a warning status message (in yellow if colors are enabled)
macro_rules! status_warn {
    ($msg:expr) => {
        $crate::terminal::Status::new()
            .bold()
            .color($crate::terminal::Color::Yellow)
            .status("warning:")
            .print_stderr($msg)
            .unwrap();
    };
    ($fmt:expr, $($arg:tt)+) => {
        $crate::terminal::status_warn!(format!($fmt, $($arg)+));
    };
}

pub(crate) use status_warn;

/// Print an error message (in red if colors are enabled)
macro_rules! status_err {
    ($msg:expr) => {
        $crate::terminal::Status::new()
            .bold()
            .color($crate::terminal::Color::Red)
            .status("error:")
            .print_stderr($msg)
            .unwrap();
    };
    ($fmt:expr, $($arg:tt)+) => {
        $crate::terminal::status_err!(format!($fmt, $($arg)+));
    };
}

pub(crate) use status_err;

/// Status message builder
#[derive(Clone, Debug, Default)]
pub(crate) struct Status {
    /// Should the status be justified?
    justified: bool,

    /// Should colors be bold?
    bold: bool,

    /// Color in which status should be displayed
    color: Option<Color>,

    /// Prefix of the status message (e.g. `Success`)
    status: Option<String>,
}

impl Status {
    /// Create a new status message with default settings
    pub(crate) fn new() -> Self {
        Self::default()
    }

    /// Justify status on display
    pub(crate) fn justified(mut self) -> Self {
        self.justified = true;
        self
    }

    /// Make colors bold
    pub(crate) fn bold(mut self) -> Self {
        self.bold = true;
        self
    }

    /// Set the colors used to display this message
    pub(crate) fn color(mut self, c: Color) -> Self {
        self.color = Some(c);
        self
    }

    /// Set a status message to display
    pub(crate) fn status(mut self, msg: impl ToString) -> Self {
        self.status = Some(msg.to_string());
        self
    }

    /// Print the given message to stdout
    pub(crate) fn print_stdout(self, msg: impl AsRef<str>) -> io::Result<()> {
        self.print(&streams().stdout, msg.as_ref())
    }

    /// Print the given message to stderr
    pub(crate) fn print_stderr(self, msg: impl AsRef<str>) -> io::Result<()> {
        self.print(&streams().stderr, msg.as_ref())
    }

    /// Print the given message
    fn print(self, stream: &StandardStream, msg: &str) -> io::Result<()> {
        let mut s = stream.lock();
        s.reset()?;
        s.set_color(ColorSpec::new().set_fg(self.color).set_bold(self.bold))?;

        if let Some(status) = self.status {
            if self.justified {
                write!(s, "{status:>12}")?;
            } else {
                write!(s, "{status}")?;
            }
        }

        s.reset()?;
        if !msg.is_empty() {
            writeln!(s, " {msg}")?;
        }
        s.flush()?;

        Ok(())
    }
}
