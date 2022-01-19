use std::fmt::{self};
use std::sync::{Arc, Mutex};
use tracing::{Event, Subscriber};
use tracing_subscriber::fmt::format::{Format, Pretty, Writer};
use tracing_subscriber::fmt::{FmtContext, FormatEvent, FormatFields, MakeWriter};
use tracing_subscriber::registry::LookupSpan;

use crate::ffi;

#[derive(Clone)]
pub struct BufWriter {
    inner: Arc<Mutex<Vec<u8>>>,
}

impl BufWriter {
    pub fn new() -> Self {
        BufWriter {
            inner: Arc::new(Mutex::new(Vec::new())),
        }
    }
}

impl std::io::Write for BufWriter {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        if let Ok(mut inner) = self.inner.try_lock() {
            return inner.write(buf);
        }
        Ok(0)
    }

    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

impl<'a> MakeWriter<'a> for BufWriter {
    type Writer = Self;

    fn make_writer(&'a self) -> Self::Writer {
        self.clone()
    }
}

pub struct CppFormatter {
    format: Format<Pretty, ()>,
    buf: BufWriter,
}

impl CppFormatter {
    pub fn new(buf: BufWriter) -> Self {
        CppFormatter {
            format: tracing_subscriber::fmt::format()
                .without_time()
                .with_ansi(false)
                .with_level(false)
                .pretty(),
            buf,
        }
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
        writer: Writer,
        event: &Event<'_>,
    ) -> fmt::Result {
        let res = self.format.format_event(ctx, writer, event);

        let level = *event.metadata().level();
        let file = event.metadata().file().unwrap_or("lib.rs");
        let line = event.metadata().line().unwrap_or(0);

        // This silently drops the log message under lock contention.
        // We expect to be called on a special single-threaded async
        // runtime, so this should not be an issue in practice.
        if let Ok(mut inner) = self.buf.inner.try_lock() {
            ffi::shim_logMessage(
                file,
                line,
                level.into(),
                String::from_utf8_lossy(&inner).trim_end(),
            );
            inner.clear();
        }

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
