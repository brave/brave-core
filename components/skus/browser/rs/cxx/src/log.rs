use std::cell::RefCell;
use tracing::{Event, Subscriber};
use tracing_subscriber::fmt::MakeWriter;
use tracing_subscriber::layer::Context;
use tracing_subscriber::prelude::*;
use tracing_subscriber::Layer;

use crate::ffi;

pub struct CxxWriter();

impl<S: Subscriber> Layer<S> for CxxWriter {
    fn on_event(&self, event: &Event<'_>, _ctx: Context<'_, S>) {
        let level = *event.metadata().level();
        let file = event.metadata().file().unwrap_or("lib.rs");
        let line = event.metadata().line().unwrap_or(0);

        CxxWriter::BUFFER.with(|inner| {
            ffi::shim_logMessage(
                file,
                line,
                level.into(),
                String::from_utf8_lossy(&inner.borrow()).trim_end(),
            );
            inner.borrow_mut().clear();
        })
    }
}

impl CxxWriter {
    thread_local! {
        static BUFFER: RefCell<Vec<u8>> = RefCell::new(Vec::new());
    }
}

fn cxx_writer() -> CxxWriter {
    CxxWriter()
}

impl std::io::Write for CxxWriter {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        CxxWriter::BUFFER.with(|inner| inner.borrow_mut().write(buf))
    }

    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

pub fn init() {
    let subscriber = tracing_subscriber::fmt()
        .with_writer(cxx_writer)
        .event_format(
            tracing_subscriber::fmt::format()
                .without_time()
                .with_ansi(false)
                .with_level(false)
                .pretty(),
        )
        .with_max_level(tracing::Level::TRACE)
        .finish();

    cxx_writer().with_subscriber(subscriber).init()
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
