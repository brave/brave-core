//! Terminal streams (STDOUT and STDIN)

use termcolor::{ColorChoice, StandardStream};

/// Terminal streams
pub struct Streams {
    /// Standard output
    pub stdout: StandardStream,

    /// Standard error
    pub stderr: StandardStream,
}

impl Streams {
    /// Create a new set of terminal streams
    pub fn new(color_choice: ColorChoice) -> Self {
        Self {
            stdout: StandardStream::stdout(color_choice),
            stderr: StandardStream::stderr(color_choice),
        }
    }
}
