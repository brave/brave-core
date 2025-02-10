#![deny(unsafe_code, missing_docs)]

/*!
Prodash is a dashboard for displaying the progress of concurrent application.

It consists of two parts

* a `Tree` to gather progress information and messages
* a terminal user interface which displays this information, along with optional free-form information provided by the application itself

Even though the `Tree` is not async, it's meant to be transparent and non-blocking performance wise, and benchmarks seem to indicate this
is indeed the case.

The **terminal user interface** seems to be the least transparent part, but can be configured to refresh less frequently.

# Terminal User Interface

By default, a TUI is provided to visualize all state. Have a look at [the example provided in the tui module](./tui/index.html).

**Please note** that it is behind the `render-tui` feature toggle, which is enabled by default.

# Logging

If the feature `progress-tree-log` is enabled (default), most calls to `progress` will also be logged.
That way, even without a terminal user interface, there will be progress messages.
Please note that logging to stdout should not be performed with this feature enabled and a terminal user interface is shown, as this will
seriously interfere with the TUI.

# A demo application

Please have a look at the [dashboard demo](https://github.com/Byron/crates-io-cli-rs/blob/master/prodash/examples/dashboard.rs).

[![asciicast](https://asciinema.org/a/301838.svg)](https://asciinema.org/a/301838)

Run it with `cargo run --example dashboard` and see what else it can do by checking out `cargo run --example dashboard -- --help`.
*/
#[cfg(feature = "atty")]
pub use atty;

#[cfg(feature = "progress-tree")]
///
pub mod tree;

///
pub mod render;

#[cfg(feature = "progress-tree-log")]
pub use log::info;
#[cfg(feature = "progress-tree-log")]
pub use log::warn;

#[cfg(any(feature = "humantime", feature = "time"))]
///
pub mod time;

///
pub mod unit;
#[doc(inline)]
pub use unit::Unit;

///
pub mod messages;
///
pub mod progress;

mod traits;
pub use traits::{
    BoxedDynNestedProgress, BoxedProgress, Count, DynNestedProgress, DynNestedProgressToNestedProgress, NestedProgress,
    Progress, Root, WeakRoot,
};

mod throughput;
pub use crate::throughput::Throughput;

#[cfg(not(feature = "progress-tree-log"))]
mod log {
    /// Stub
    #[macro_export(local_inner_macros)]
    macro_rules! warn {
        (target: $target:expr, $($arg:tt)+) => {};
        ($($arg:tt)+) => {};
    }
    /// Stub
    #[macro_export(local_inner_macros)]
    macro_rules! info {
        (target: $target:expr, $($arg:tt)+) => {};
        ($($arg:tt)+) => {};
    }
}
