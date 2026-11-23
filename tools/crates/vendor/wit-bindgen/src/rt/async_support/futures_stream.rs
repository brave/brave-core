use super::stream_support::{RawStreamReader, StreamOps, StreamVtable};
use alloc::boxed::Box;
use core::{
    future::Future,
    pin::Pin,
    task::{Context, Poll},
};

/// A wrapper around [`RawStreamReader`] that implements [`futures::Stream`].
///
/// Obtain one via [`RawStreamReader::into_stream`] or
/// [`RawStreamReaderStream::new`].
pub struct RawStreamReaderStream<O: StreamOps + 'static> {
    state: StreamAdapterState<O>,
}

// SAFETY: No field is structurally pinned. The inner `Pin<Box<dyn Future>>`
// is itself `Unpin`, and `RawStreamReader` is only stored when idle.
impl<O: StreamOps + 'static> Unpin for RawStreamReaderStream<O> {}

/// Convenience alias for the common vtable-based case.
pub type StreamReaderStream<T> = RawStreamReaderStream<&'static StreamVtable<T>>;

type ReadNextFut<O> =
    Pin<Box<dyn Future<Output = (RawStreamReader<O>, Option<<O as StreamOps>::Payload>)>>>;

enum StreamAdapterState<O: StreamOps + 'static> {
    /// The reader is idle and ready for the next read.
    Idle(RawStreamReader<O>),
    /// A read is in progress.
    Reading(ReadNextFut<O>),
    /// The stream has been exhausted.
    Complete,
}

impl<O: StreamOps + 'static> RawStreamReaderStream<O> {
    /// Create a new [`futures::Stream`] wrapper from a [`RawStreamReader`].
    pub fn new(reader: RawStreamReader<O>) -> Self {
        Self {
            state: StreamAdapterState::Idle(reader),
        }
    }

    /// Recover the underlying [`RawStreamReader`], if no read is in flight.
    ///
    /// Returns `None` when a read is currently in progress or the stream has
    /// already finished.
    pub fn into_inner(self) -> Option<RawStreamReader<O>> {
        match self.state {
            StreamAdapterState::Idle(reader) => Some(reader),
            _ => None,
        }
    }
}

impl<O: StreamOps + 'static> futures::stream::Stream for RawStreamReaderStream<O> {
    type Item = O::Payload;

    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        // All variants of `StreamAdapterState` are `Unpin`, so `Pin<&mut Self>`
        // can be freely projected.
        loop {
            match core::mem::replace(&mut self.state, StreamAdapterState::Complete) {
                StreamAdapterState::Idle(mut reader) => {
                    let fut: ReadNextFut<O> = Box::pin(async move {
                        let item = reader.next().await;
                        (reader, item)
                    });
                    self.state = StreamAdapterState::Reading(fut);
                    // Loop to immediately poll the new future.
                }
                StreamAdapterState::Reading(mut fut) => match fut.as_mut().poll(cx) {
                    Poll::Pending => {
                        self.state = StreamAdapterState::Reading(fut);
                        return Poll::Pending;
                    }
                    Poll::Ready((reader, Some(item))) => {
                        self.state = StreamAdapterState::Idle(reader);
                        return Poll::Ready(Some(item));
                    }
                    Poll::Ready((_reader, None)) => {
                        self.state = StreamAdapterState::Complete;
                        return Poll::Ready(None);
                    }
                },
                StreamAdapterState::Complete => {
                    self.state = StreamAdapterState::Complete;
                    return Poll::Ready(None);
                }
            }
        }
    }
}

impl<O: StreamOps + 'static> RawStreamReader<O> {
    /// Convert this reader into a [`futures::Stream`].
    pub fn into_stream(self) -> RawStreamReaderStream<O> {
        RawStreamReaderStream::new(self)
    }
}
