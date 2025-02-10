//! Logging configuration

/// Tracing configuration
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Config {
    pub(super) filter: String,
}

impl Config {
    /// Create a config for verbose output.
    pub fn verbose() -> Self {
        "debug".to_owned().into()
    }
}

impl Default for Config {
    fn default() -> Self {
        std::env::var("RUST_LOG")
            .unwrap_or("info".to_owned())
            .into()
    }
}

impl From<String> for Config {
    fn from(filter: String) -> Self {
        Self { filter }
    }
}
