#[cfg(feature = "signal-hook")]
use std::sync::Arc;
use std::{
    io,
    ops::RangeInclusive,
    sync::atomic::{AtomicBool, Ordering},
    time::Duration,
};

use crate::{progress, render::line::draw, Throughput, WeakRoot};

/// Options used for configuring a [line renderer][render()].
#[derive(Clone)]
pub struct Options {
    /// If true, _(default true)_, we assume the output stream belongs to a terminal.
    ///
    /// If false, we won't print any live progress, only log messages.
    pub output_is_terminal: bool,

    /// If true, _(default: true)_ we will display color. You should use `output_is_terminal && crosstermion::should_colorize()`
    /// to determine this value.
    ///
    /// Please note that you can enforce color even if the output stream is not connected to a terminal by setting
    /// this field to true.
    pub colored: bool,

    /// If true, _(default: false)_, a timestamp will be shown before each message.
    pub timestamp: bool,

    /// The amount of columns and rows to use for drawing. Defaults to (80, 20).
    pub terminal_dimensions: (u16, u16),

    /// If true, _(default: false)_, the cursor will be hidden for a more visually appealing display.
    ///
    /// Please note that you must make sure the line renderer is properly shut down to restore the previous cursor
    /// settings. See the `signal-hook` documentation in the README for more information.
    pub hide_cursor: bool,

    /// If true, (default false), we will keep track of the previous progress state to derive
    /// continuous throughput information from. Throughput will only show for units which have
    /// explicitly enabled it, it is opt-in.
    ///
    /// This comes at the cost of additional memory and CPU time.
    pub throughput: bool,

    /// If set, specify all levels that should be shown. Otherwise all available levels are shown.
    ///
    /// This is useful to filter out high-noise lower level progress items in the tree.
    pub level_filter: Option<RangeInclusive<progress::key::Level>>,

    /// If set, progress will only actually be shown after the given duration. Log messages will always be shown without delay.
    ///
    /// This option can be useful to not enforce progress for short actions, causing it to flicker.
    /// Please note that this won't affect display of messages, which are simply logged.
    pub initial_delay: Option<Duration>,

    /// The amount of frames to draw per second. If below 1.0, it determines the amount of seconds between the frame.
    ///
    /// *e.g.* 1.0/4.0 is one frame every 4 seconds.
    pub frames_per_second: f32,

    /// If true (default: true), we will keep waiting for progress even after we encountered an empty list of drawable progress items.
    ///
    /// Please note that you should add at least one item to the `prodash::Tree` before launching the application or else
    /// risk a race causing nothing to be rendered at all.
    pub keep_running_if_progress_is_empty: bool,
}

/// The kind of stream to use for auto-configuration.
pub enum StreamKind {
    /// Standard output
    Stdout,
    /// Standard error
    Stderr,
}

/// Convenience
impl Options {
    /// Automatically configure (and overwrite) the following fields based on terminal configuration.
    ///
    /// * output_is_terminal
    /// * colored
    /// * terminal_dimensions
    /// * hide-cursor (based on presence of 'signal-hook' feature.
    #[cfg(feature = "render-line-autoconfigure")]
    pub fn auto_configure(mut self, output: StreamKind) -> Self {
        self.output_is_terminal = match output {
            StreamKind::Stdout => is_terminal::is_terminal(std::io::stdout()),
            StreamKind::Stderr => is_terminal::is_terminal(std::io::stderr()),
        };
        self.colored = self.output_is_terminal && crosstermion::color::allowed();
        self.terminal_dimensions = crosstermion::terminal::size().unwrap_or((80, 20));
        #[cfg(feature = "signal-hook")]
        self.auto_hide_cursor();
        self
    }
    #[cfg(all(feature = "render-line-autoconfigure", feature = "signal-hook"))]
    fn auto_hide_cursor(&mut self) {
        self.hide_cursor = true;
    }
    #[cfg(not(feature = "render-line-autoconfigure"))]
    /// No-op - only available with the `render-line-autoconfigure` feature toggle.
    pub fn auto_configure(self, _output: StreamKind) -> Self {
        self
    }
}

impl Default for Options {
    fn default() -> Self {
        Options {
            output_is_terminal: true,
            colored: true,
            timestamp: false,
            terminal_dimensions: (80, 20),
            hide_cursor: false,
            level_filter: None,
            initial_delay: None,
            frames_per_second: 6.0,
            throughput: false,
            keep_running_if_progress_is_empty: true,
        }
    }
}

/// A handle to the render thread, which when dropped will instruct it to stop showing progress.
pub struct JoinHandle {
    inner: Option<std::thread::JoinHandle<io::Result<()>>>,
    connection: std::sync::mpsc::SyncSender<Event>,
    // If we disconnect before sending a Quit event, the selector continuously informs about the 'Disconnect' state
    disconnected: bool,
}

impl JoinHandle {
    /// `detach()` and `forget()` to remove any effects associated with this handle.
    pub fn detach(mut self) {
        self.disconnect();
        self.forget();
    }
    /// Remove the handles capability to instruct the render thread to stop, but it will still wait for it
    /// if dropped.
    /// Use `forget()` if it should not wait for the render thread anymore.
    pub fn disconnect(&mut self) {
        self.disconnected = true;
    }
    /// Remove the handles capability to `join()` by forgetting the threads handle
    pub fn forget(&mut self) {
        self.inner.take();
    }
    /// Wait for the thread to shutdown naturally, for example because there is no more progress to display
    pub fn wait(mut self) {
        self.inner.take().and_then(|h| h.join().ok());
    }
    /// Send the shutdown signal right after one last redraw
    pub fn shutdown(&mut self) {
        if !self.disconnected {
            self.connection.send(Event::Tick).ok();
            self.connection.send(Event::Quit).ok();
        }
    }
    /// Send the signal to shutdown and wait for the thread to be shutdown.
    pub fn shutdown_and_wait(mut self) {
        self.shutdown();
        self.wait();
    }
}

impl Drop for JoinHandle {
    fn drop(&mut self) {
        self.shutdown();
        self.inner.take().and_then(|h| h.join().ok());
    }
}

#[derive(Debug)]
enum Event {
    Tick,
    Quit,
    #[cfg(feature = "signal-hook")]
    Resize(u16, u16),
}

/// Write a line-based representation of `progress` to `out` which is assumed to be a terminal.
///
/// Configure it with `config`, see the [`Options`] for details.
pub fn render(
    mut out: impl io::Write + Send + 'static,
    progress: impl WeakRoot + Send + 'static,
    Options {
        output_is_terminal,
        colored,
        timestamp,
        level_filter,
        terminal_dimensions,
        initial_delay,
        frames_per_second,
        keep_running_if_progress_is_empty,
        hide_cursor,
        throughput,
    }: Options,
) -> JoinHandle {
    #[cfg_attr(not(feature = "signal-hook"), allow(unused_mut))]
    let mut config = draw::Options {
        level_filter,
        terminal_dimensions,
        keep_running_if_progress_is_empty,
        output_is_terminal,
        colored,
        timestamp,
        hide_cursor,
    };

    let (event_send, event_recv) = std::sync::mpsc::sync_channel::<Event>(1);
    let show_cursor = possibly_hide_cursor(&mut out, hide_cursor && output_is_terminal);
    static SHOW_PROGRESS: AtomicBool = AtomicBool::new(false);
    #[cfg(feature = "signal-hook")]
    let term_signal_received: Arc<AtomicBool> = Arc::new(AtomicBool::new(false));
    #[cfg(feature = "signal-hook")]
    let terminal_resized: Arc<AtomicBool> = Arc::new(AtomicBool::new(false));
    #[cfg(feature = "signal-hook")]
    {
        for sig in signal_hook::consts::TERM_SIGNALS {
            signal_hook::flag::register(*sig, term_signal_received.clone()).ok();
        }

        #[cfg(unix)]
        signal_hook::flag::register(signal_hook::consts::SIGWINCH, terminal_resized.clone()).ok();
    }

    let handle = std::thread::Builder::new()
        .name("render-line-eventloop".into())
        .spawn({
            let tick_send = event_send.clone();
            move || {
                {
                    let initial_delay = initial_delay.unwrap_or_default();
                    SHOW_PROGRESS.store(initial_delay == Duration::default(), Ordering::Relaxed);
                    if !SHOW_PROGRESS.load(Ordering::Relaxed) {
                        std::thread::Builder::new()
                            .name("render-line-progress-delay".into())
                            .spawn(move || {
                                std::thread::sleep(initial_delay);
                                SHOW_PROGRESS.store(true, Ordering::Relaxed);
                            })
                            .ok();
                    }
                }

                let mut state = draw::State::default();
                if throughput {
                    state.throughput = Some(Throughput::default());
                }
                let secs = 1.0 / frames_per_second;
                let _ticker = std::thread::Builder::new()
                    .name("render-line-ticker".into())
                    .spawn(move || loop {
                        #[cfg(feature = "signal-hook")]
                        {
                            if term_signal_received.load(Ordering::SeqCst) {
                                tick_send.send(Event::Quit).ok();
                                break;
                            }
                            if terminal_resized.load(Ordering::SeqCst) {
                                terminal_resized.store(false, Ordering::SeqCst);
                                if let Ok((x, y)) = crosstermion::terminal::size() {
                                    tick_send.send(Event::Resize(x, y)).ok();
                                }
                            }
                        }
                        if tick_send.send(Event::Tick).is_err() {
                            break;
                        }
                        std::thread::sleep(Duration::from_secs_f32(secs));
                    })
                    .expect("starting a thread works");

                for event in event_recv {
                    match event {
                        #[cfg(feature = "signal-hook")]
                        Event::Resize(x, y) => {
                            config.terminal_dimensions = (x, y);
                            draw::all(&mut out, SHOW_PROGRESS.load(Ordering::Relaxed), &mut state, &config)?;
                        }
                        Event::Tick => match progress.upgrade() {
                            Some(progress) => {
                                let has_changed = state.update_from_progress(&progress);
                                draw::all(
                                    &mut out,
                                    SHOW_PROGRESS.load(Ordering::Relaxed) && has_changed,
                                    &mut state,
                                    &config,
                                )?;
                            }
                            None => {
                                state.clear();
                                draw::all(&mut out, SHOW_PROGRESS.load(Ordering::Relaxed), &mut state, &config)?;
                                break;
                            }
                        },
                        Event::Quit => {
                            state.clear();
                            draw::all(&mut out, SHOW_PROGRESS.load(Ordering::Relaxed), &mut state, &config)?;
                            break;
                        }
                    }
                }

                if show_cursor {
                    crosstermion::execute!(out, crosstermion::cursor::Show).ok();
                }

                // One day we might try this out on windows, but let's not risk it now.
                #[cfg(unix)]
                write!(out, "\x1b[2K\r").ok(); // clear the last line.
                Ok(())
            }
        })
        .expect("starting a thread works");

    JoinHandle {
        inner: Some(handle),
        connection: event_send,
        disconnected: false,
    }
}

// Not all configurations actually need it to be mut, but those with the 'signal-hook' feature do
#[allow(unused_mut)]
fn possibly_hide_cursor(out: &mut impl io::Write, mut hide_cursor: bool) -> bool {
    if hide_cursor {
        crosstermion::execute!(out, crosstermion::cursor::Hide).is_ok()
    } else {
        false
    }
}
