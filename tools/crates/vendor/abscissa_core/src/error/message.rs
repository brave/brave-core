//! Error messages

use std::{
    error::Error,
    fmt::{self, Display},
    string::ToString,
};

/// Error message type: provide additional context with a string.
///
/// This is generally discouraged whenever possible as it will complicate
/// future I18n support. However, it can be useful for things with
/// language-independent string representations for error contexts.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Message(String);

impl Message {
    /// Create a new error message
    pub fn new(msg: impl ToString) -> Self {
        Message(msg.to_string())
    }
}

impl AsRef<str> for Message {
    fn as_ref(&self) -> &str {
        self.0.as_ref()
    }
}

impl Display for Message {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.as_ref())
    }
}

impl Error for Message {}

impl From<String> for Message {
    fn from(string: String) -> Message {
        Message(string)
    }
}
