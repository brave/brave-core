// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::collections::VecDeque;
use std::fmt::{self};
use std::sync::Arc;
use std::sync::Mutex;

use chrono::{DateTime, Utc};
use tracing::{Event, Level, Subscriber};
use tracing_subscriber::fmt::format::FormatFields;
use tracing_subscriber::fmt::FormattedFields;
use tracing_subscriber::registry::LookupSpan;
use tracing_subscriber::Layer;

/// Represents a log entry with a timestamp, verbosity level, and message.
pub struct LogEntry {
    /// The timestamp when the log entry was created.
    logged_at: DateTime<Utc>,
    /// The verbosity level of the log entry.
    verbosity: Level,
    /// The message of the log entry.
    message: String,
}

/// A circular buffer for storing `LogEntry`s for the duration of a span.
pub struct SpanLogBuffer {
    /// The buffer of log entries.
    buf: VecDeque<LogEntry>,
    /// The maximum number of log entries that can be stored in the buffer.
    capacity: usize,
    /// Indicates whether an error has occurred within this span
    error_occured: bool,
}

impl SpanLogBuffer {
    /// Initialize a new SpanLogBuffer with the given capacity
    fn new(capacity: usize) -> Self {
        Self {
            // A double-ended queue implemented with a growable ring buffer to store the log entries
            buf: VecDeque::new(),
            // A new span is assumed to start without error :)
            error_occured: false,
            // The maximum length of the buffer
            capacity,
        }
    }

    /// Add the log entry to the buffer, overwriting an entry if the capacity
    /// has been reached
    fn push_back(&mut self, entry: LogEntry) {
        // If the entry's verbosity is ERROR, set the `error_occured` flag to true
        if entry.verbosity == Level::ERROR || entry.verbosity == Level::WARN {
            self.error_occured = true;
        }

        // If the buffer's length is greater than or equal to the capacity, remove the
        // first entry
        if self.buf.len() >= self.capacity {
            self.buf.pop_front();
        }

        // Add the new entry to the end of the buffer
        self.buf.push_back(entry);
    }
}

/// A tracing layer which stores formatted log entries in an in-memory ring
/// buffer.
///
/// Log entries for events within in-flight spans are temporarily enqueued in
/// secondary per-span buffers. Upon completion of a span, the buffered events
/// are filtered based on whether an error occured within execution of the span.
/// If an error occurred, then events of all levels including TRACE are
/// retained, otherwise only INFO and below are retained.
pub struct RingBufferLayer<N> {
    /// The buffer for storing log entries.
    buf: Arc<Mutex<VecDeque<char>>>,
    /// The capacity for the main log buffer.
    main_capacity: usize,
    /// The capacity for the temporary per-span LogEntry buffer.
    span_capacity: usize,
    /// The formatter for formatting log entries.
    formatter: SimpleFormatter<N>,
    /// In test mode, this field holds the mocked date and time.
    #[cfg(test)]
    pub now: DateTime<Utc>,
}

impl<N> RingBufferLayer<N> {
    /// The `new` function is a constructor for the `Log` struct. It creates a
    /// new instance of `Log` with the provided parameters.
    #[cfg(not(test))]
    pub fn new(
        buf: Arc<Mutex<VecDeque<char>>>,
        fmt_fields: N,
        main_capacity: usize,
        span_capacity: usize,
    ) -> Self {
        Self { buf, formatter: SimpleFormatter { fmt_fields }, main_capacity, span_capacity }
    }

    /// Same as above, but with an additional `now` parameter for test
    /// configurations.
    #[cfg(test)]
    pub fn new(
        buf: Arc<Mutex<VecDeque<char>>>,
        fmt_fields: N,
        main_capacity: usize,
        span_capacity: usize,
        now: DateTime<Utc>,
    ) -> Self {
        Self { buf, formatter: SimpleFormatter { fmt_fields }, main_capacity, span_capacity, now }
    }

    /// Returns the current date and time in UTC.
    #[cfg(not(test))]
    pub fn now(&self) -> DateTime<Utc> {
        Utc::now()
    }

    /// In test mode, this function returns the stored `now` value for mocking
    /// purposes.
    #[cfg(test)]
    pub fn now(&self) -> DateTime<Utc> {
        self.now
    }
}

/// A simple tracing event formatter.
///
/// Ideally we'd re-use tracing_subscriber::fmt or just implement the
/// FormatEvent trait but we have no way of constructing FmtContext so we
/// instead take Context. This is essentially 1:1 with the FormatEvent trait
/// example otherwise.
struct SimpleFormatter<N> {
    fmt_fields: N,
}

impl<N> SimpleFormatter<N> {
    fn format_event<S>(
        &self,
        ctx: tracing_subscriber::layer::Context<'_, S>,
        writer: &mut dyn fmt::Write,
        event: &Event<'_>,
    ) -> fmt::Result
    where
        S: Subscriber + for<'a> LookupSpan<'a>,
        N: for<'a> FormatFields<'a> + 'static,
    {
        // Write level and target
        let level = *event.metadata().level();
        let target = event.metadata().target();
        write!(writer, "{} {}: ", level, target,)?;

        // Write spans and fields of each span
        if let Some(leaf) = ctx.lookup_current() {
            for span in leaf.scope().from_root() {
                write!(writer, "{}", span.name())?;

                let ext = span.extensions();

                // `FormattedFields` is a a formatted representation of the span's
                // fields, which is stored in its extensions by the `fmt` layer's
                // `new_span` method. The fields will have been formatted
                // by the same field formatter that's provided to the event
                // formatter in the `FmtContext`.
                let fields = &ext.get::<FormattedFields<N>>().expect("will never be `None`");

                if !fields.is_empty() {
                    write!(writer, "{{{}}}", fields)?;
                }
                write!(writer, ": ")?;
            }
        }

        // Write fields on the event
        self.fmt_fields.format_fields(writer, event)?;

        writeln!(writer)
    }
}

impl<S, N> Layer<S> for RingBufferLayer<N>
where
    S: tracing::Subscriber + for<'a> tracing_subscriber::registry::LookupSpan<'a>,
    N: for<'a> FormatFields<'a> + 'static,
{
    /// This function is called when a span is created.
    fn new_span(
        &self,
        attrs: &tracing::span::Attributes<'_>,
        id: &tracing::span::Id,
        ctx: tracing_subscriber::layer::Context<'_, S>,
    ) {
        if let Some(span) = ctx.span(id) {
            let mut extensions = span.extensions_mut();
            // create a span log buffer and stash in extensions if this is a root span
            // and one does not already exist
            if span.parent().is_none() {
                if extensions.get_mut::<SpanLogBuffer>().is_none() {
                    extensions.insert(SpanLogBuffer::new(self.span_capacity));
                }
            }
            // format the span fields and stash in extensions if they haven't already been
            if extensions.get_mut::<FormattedFields<N>>().is_none() {
                let mut buf = String::new();
                if self.formatter.fmt_fields.format_fields(&mut buf, attrs).is_ok() {
                    let fmt_fields = FormattedFields::<N>::new(buf);
                    extensions.insert(fmt_fields);
                }
            }
        }
    }

    /// This function is called when a span is closed.
    fn on_close(&self, id: tracing::span::Id, ctx: tracing_subscriber::layer::Context<'_, S>) {
        if let Some(span) = ctx.span(&id) {
            let mut extensions = span.extensions_mut();
            if let Some(log_buffer) = extensions.remove::<SpanLogBuffer>() {
                // the final verbosity level for the span is determined by whether an error has
                // occurred.
                let mut level = Level::INFO;
                if log_buffer.error_occured {
                    level = Level::TRACE;
                }
                // the span buffer is processed by filtering out log messages with a higher
                // verbosity level. after formatting, remaining log entries are
                // added to the main buffer
                if let Ok(mut buf) = self.buf.lock() {
                    let chars: Vec<_> = log_buffer
                        .buf
                        .into_iter()
                        .filter(|x| x.verbosity <= level)
                        .flat_map(|x| {
                            format!("{} {}", x.logged_at.to_string(), x.message)
                                .chars()
                                .collect::<Vec<_>>()
                                .into_iter()
                        })
                        .collect();

                    // if the buffer's new length will be greater than the capacity, first remove
                    // entries to ensure we will not exceed the capacity
                    let new_len = buf.len() + chars.len();
                    if new_len > self.main_capacity {
                        for _ in 0..new_len - self.main_capacity {
                            buf.pop_front();
                        }
                    }

                    buf.extend(chars);
                }
            }
        }
    }

    /// This function is called when an event is created.
    fn on_event(&self, event: &tracing::Event<'_>, ctx: tracing_subscriber::layer::Context<'_, S>) {
        if let Some(scope) = ctx.event_scope(event) {
            if let Some(root) = scope.from_root().next() {
                let id = root.id();

                let mut buf = String::new();
                let writer = &mut buf;

                if self.formatter.format_event(ctx.clone(), writer, event).is_ok() {
                    if let Some(span) = ctx.span(&id) {
                        let mut extensions = span.extensions_mut();
                        // after formatting the event we insert it into the root span log buffer
                        if let Some(log_buffer) = extensions.get_mut::<SpanLogBuffer>() {
                            let entry = LogEntry {
                                verbosity: *event.metadata().level(),
                                message: buf,
                                logged_at: self.now(),
                            };
                            log_buffer.push_back(entry);
                        }
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod test {
    use std::collections::VecDeque;
    use std::sync::Arc;
    use std::sync::Mutex;

    use async_std::task;
    use chrono::{NaiveDate, TimeZone, Utc};
    use tracing::{debug, error, info, instrument, trace, warn};
    use tracing_subscriber::fmt::format::DefaultFields;
    use tracing_subscriber::layer::SubscriberExt;
    use tracing_subscriber::util::SubscriberInitExt;

    use crate::log::RingBufferLayer;

    #[instrument]
    fn log_error() {
        trace!("trace");
        error!("error");
    }

    #[instrument]
    fn log_warn() {
        debug!("debug");
        warn!("warn");
    }

    #[instrument]
    fn log_info() {
        debug!("debug");
        info!("info");
    }

    #[test]
    fn test_log_buffer() {
        let now = Utc.from_utc_datetime(
            &NaiveDate::from_ymd_opt(2016, 7, 8).unwrap().and_hms_opt(9, 10, 11).unwrap(),
        );

        let buf = Arc::new(Mutex::new(VecDeque::new()));
        tracing_subscriber::registry()
            .with(RingBufferLayer::new(buf.clone(), DefaultFields::new(), 1023, 1023, now))
            .init();

        log_info();
        log_warn();
        log_error();

        task::block_on(async {
            if let Ok(buf) = buf.lock() {
                let expected = r###"2016-07-08 09:10:11 UTC INFO skus::log::test: log_info: info
2016-07-08 09:10:11 UTC DEBUG skus::log::test: log_warn: debug
2016-07-08 09:10:11 UTC WARN skus::log::test: log_warn: warn
2016-07-08 09:10:11 UTC TRACE skus::log::test: log_error: trace
2016-07-08 09:10:11 UTC ERROR skus::log::test: log_error: error
"###;

                let out: String = buf.iter().collect();
                assert_eq!(out, expected);
            }
        })
    }
}
