use std::fmt::{self};
use tracing::{Event, Subscriber};
use tracing_subscriber::fmt::format::{Format, Pretty};
use tracing_subscriber::fmt::{FmtContext, FormatEvent, FormatFields};
use tracing_subscriber::registry::LookupSpan;

use crate::ffi;

pub struct CppFormatter(Format<Pretty, ()>);

impl CppFormatter {
    pub fn new() -> Self {
        CppFormatter(
            tracing_subscriber::fmt::format()
                .without_time()
                .with_ansi(false)
                .with_level(false)
                .pretty(),
        )
    }
}

impl<S, N> FormatEvent<S, N> for CppFormatter
where
    S: Subscriber + for<'a> LookupSpan<'a>,
    N: for<'a> FormatFields<'a> + 'static,
{
    fn format_event(
        &self,
        ctx: &FmtContext<'_, S, N>,
        _writer: &mut dyn fmt::Write,
        event: &Event<'_>,
    ) -> fmt::Result {
        let mut buf = String::new();
        let writer = &mut buf;

        let res = self.0.format_event(ctx, writer, event);

        let level = *event.metadata().level();
        let file = event.metadata().file().unwrap_or("lib.rs");
        let line = event.metadata().line().unwrap_or(0);

        ffi::shim_logMessage(file, line, level.into(), buf.trim_end());

        res
    }
}

impl From<tracing::Level> for ffi::TracingLevel {
    fn from(level: tracing::Level) -> Self {
        match level {
            tracing::Level::ERROR => ffi::TracingLevel::Error,
            tracing::Level::WARN => ffi::TracingLevel::Warn,
            tracing::Level::INFO => ffi::TracingLevel::Info,
            tracing::Level::DEBUG => ffi::TracingLevel::Debug,
            tracing::Level::TRACE => ffi::TracingLevel::Trace,
        }
    }
}
