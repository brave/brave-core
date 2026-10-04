use std::{future::Future, pin::Pin, task::Poll, time::Duration};

use async_io::Timer;

/// Returns a stream of 'ticks', each being duration `dur` apart.
///
/// Can be useful to provide the TUI with additional events in regular intervals,
/// when using the [`tui::render_with_input(…events)`](./fn.render_with_input.html) function.
pub fn ticker(dur: Duration) -> impl futures_core::Stream<Item = ()> {
    let mut delay = Timer::after(dur);
    futures_lite::stream::poll_fn(move |ctx| {
        let res = Pin::new(&mut delay).poll(ctx);
        match res {
            Poll::Pending => Poll::Pending,
            Poll::Ready(_) => {
                delay = Timer::after(dur);
                Poll::Ready(Some(()))
            }
        }
    })
}

pub const VERTICAL_LINE: &str = "│";

pub use tui_react::{draw_text_nowrap_fn, draw_text_with_ellipsis_nowrap, util::*};
