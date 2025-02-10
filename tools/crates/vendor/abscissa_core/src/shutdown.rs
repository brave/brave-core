//! Application shutdown support

/// Types of shutdown recognized by Abscissa
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq)]
pub enum Shutdown {
    /// Graceful shutdowns may take prolonged periods of time, allowing
    /// components to take their time to ensure shutdowns occur cleanly
    /// (e.g. draining currently active traffic rather than closing sockets)
    Graceful,

    /// Forced shutdowns indicate the program's user has requested it terminate
    /// immediately. Components receiving this kind of shutdown should do only
    /// critical cleanup tasks which can be completed quickly.
    Forced,

    /// This shutdown type is a "best effort" to communicate that the
    /// application has suffered from a critical error and is in the process
    /// of exiting. Components may use this to do crash reporting prior
    /// to the application exit, as well as any other cleanup deemed suitable
    /// within a crashing application.
    Crash,
}
