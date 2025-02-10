use std::{
    collections::{hash_map::DefaultHasher, VecDeque},
    hash::{Hash, Hasher},
    io,
    ops::RangeInclusive,
    sync::atomic::Ordering,
};

use crosstermion::{
    ansi_term::{ANSIString, ANSIStrings, Color, Style},
    color,
};
use unicode_width::UnicodeWidthStr;

use crate::{
    messages::{Message, MessageCopyState, MessageLevel},
    progress::{self, Value},
    unit, Root, Throughput,
};

#[derive(Default)]
pub struct State {
    tree: Vec<(progress::Key, progress::Task)>,
    tree_hash: u64,
    messages: Vec<Message>,
    for_next_copy: Option<MessageCopyState>,
    /// The size of the message origin, tracking the terminal height so things potentially off screen don't influence width anymore.
    message_origin_size: VecDeque<usize>,
    /// The maximum progress midpoint (point till progress bar starts) seen at the last tick
    last_progress_midpoint: Option<u16>,
    /// The amount of blocks per line we have written last time.
    blocks_per_line: VecDeque<u16>,
    pub throughput: Option<Throughput>,
}

impl State {
    pub(crate) fn update_from_progress(&mut self, progress: &impl Root) -> bool {
        progress.sorted_snapshot(&mut self.tree);
        let mut hasher = DefaultHasher::new();
        self.tree.hash(&mut hasher);
        let cur_hash = hasher.finish();

        self.for_next_copy = progress
            .copy_new_messages(&mut self.messages, self.for_next_copy.take())
            .into();
        let changed = self.tree_hash != cur_hash;
        self.tree_hash = cur_hash;
        changed
    }
    pub(crate) fn clear(&mut self) {
        self.tree.clear();
        self.messages.clear();
        self.for_next_copy.take();
    }
}

pub struct Options {
    pub level_filter: Option<RangeInclusive<progress::key::Level>>,
    pub terminal_dimensions: (u16, u16),
    pub keep_running_if_progress_is_empty: bool,
    pub output_is_terminal: bool,
    pub colored: bool,
    pub timestamp: bool,
    pub hide_cursor: bool,
}

fn messages(
    out: &mut impl io::Write,
    state: &mut State,
    colored: bool,
    max_height: usize,
    timestamp: bool,
) -> io::Result<()> {
    let mut brush = color::Brush::new(colored);
    fn to_color(level: MessageLevel) -> Color {
        use crate::messages::MessageLevel::*;
        match level {
            Info => Color::White,
            Success => Color::Green,
            Failure => Color::Red,
        }
    }
    let mut tokens: Vec<ANSIString<'_>> = Vec::with_capacity(6);
    let mut current_maximum = state.message_origin_size.iter().max().cloned().unwrap_or(0);
    for Message {
        time,
        level,
        origin,
        message,
    } in &state.messages
    {
        tokens.clear();
        let blocks_drawn_during_previous_tick = state.blocks_per_line.pop_front().unwrap_or(0);
        let message_block_len = origin.width();
        current_maximum = current_maximum.max(message_block_len);
        if state.message_origin_size.len() == max_height {
            state.message_origin_size.pop_front();
        }
        state.message_origin_size.push_back(message_block_len);

        let color = to_color(*level);
        tokens.push(" ".into());
        if timestamp {
            tokens.push(
                brush
                    .style(color.dimmed().on(Color::Yellow))
                    .paint(crate::time::format_time_for_messages(*time)),
            );
            tokens.push(Style::default().paint(" "));
        } else {
            tokens.push("".into());
        };
        tokens.push(brush.style(Style::default().dimmed()).paint(format!(
            "{:>fill_size$}{}",
            "",
            origin,
            fill_size = current_maximum - message_block_len,
        )));
        tokens.push(" ".into());
        tokens.push(brush.style(color.bold()).paint(message));
        let message_block_count = block_count_sans_ansi_codes(&tokens);
        write!(out, "{}", ANSIStrings(tokens.as_slice()))?;

        if blocks_drawn_during_previous_tick > message_block_count {
            newline_with_overdraw(out, &tokens, blocks_drawn_during_previous_tick)?;
        } else {
            writeln!(out)?;
        }
    }
    Ok(())
}

pub fn all(out: &mut impl io::Write, show_progress: bool, state: &mut State, config: &Options) -> io::Result<()> {
    if !config.keep_running_if_progress_is_empty && state.tree.is_empty() {
        return Err(io::Error::new(io::ErrorKind::Other, "stop as progress is empty"));
    }
    messages(
        out,
        state,
        config.colored,
        config.terminal_dimensions.1 as usize,
        config.timestamp,
    )?;

    if show_progress && config.output_is_terminal {
        if let Some(tp) = state.throughput.as_mut() {
            tp.update_elapsed();
        }
        let level_range = config
            .level_filter
            .clone()
            .unwrap_or(RangeInclusive::new(0, progress::key::Level::max_value()));
        let lines_to_be_drawn = state
            .tree
            .iter()
            .filter(|(k, _)| level_range.contains(&k.level()))
            .count();
        if state.blocks_per_line.len() < lines_to_be_drawn {
            state.blocks_per_line.resize(lines_to_be_drawn, 0);
        }
        let mut tokens: Vec<ANSIString<'_>> = Vec::with_capacity(4);
        let mut max_midpoint = 0;
        for ((key, value), ref mut blocks_in_last_iteration) in state
            .tree
            .iter()
            .filter(|(k, _)| level_range.contains(&k.level()))
            .zip(state.blocks_per_line.iter_mut())
        {
            max_midpoint = max_midpoint.max(
                format_progress(
                    key,
                    value,
                    config.terminal_dimensions.0,
                    config.colored,
                    state.last_progress_midpoint,
                    state
                        .throughput
                        .as_mut()
                        .and_then(|tp| tp.update_and_get(key, value.progress.as_ref())),
                    &mut tokens,
                )
                .unwrap_or(0),
            );
            write!(out, "{}", ANSIStrings(tokens.as_slice()))?;

            **blocks_in_last_iteration = newline_with_overdraw(out, &tokens, **blocks_in_last_iteration)?;
        }
        if let Some(tp) = state.throughput.as_mut() {
            tp.reconcile(&state.tree);
        }
        state.last_progress_midpoint = Some(max_midpoint);
        // overwrite remaining lines that we didn't touch naturally
        let lines_drawn = lines_to_be_drawn;
        if state.blocks_per_line.len() > lines_drawn {
            for blocks_in_last_iteration in state.blocks_per_line.iter().skip(lines_drawn) {
                writeln!(out, "{:>width$}", "", width = *blocks_in_last_iteration as usize)?;
            }
            // Move cursor back to end of the portion we have actually drawn
            crosstermion::execute!(out, crosstermion::cursor::MoveUp(state.blocks_per_line.len() as u16))?;
            state.blocks_per_line.resize(lines_drawn, 0);
        } else if lines_drawn > 0 {
            crosstermion::execute!(out, crosstermion::cursor::MoveUp(lines_drawn as u16))?;
        }
    }
    Ok(())
}

/// Must be called directly after `tokens` were drawn, without newline. Takes care of adding the newline.
fn newline_with_overdraw(
    out: &mut impl io::Write,
    tokens: &[ANSIString<'_>],
    blocks_in_last_iteration: u16,
) -> io::Result<u16> {
    let current_block_count = block_count_sans_ansi_codes(tokens);
    if blocks_in_last_iteration > current_block_count {
        // fill to the end of line to overwrite what was previously there
        writeln!(
            out,
            "{:>width$}",
            "",
            width = (blocks_in_last_iteration - current_block_count) as usize
        )?;
    } else {
        writeln!(out)?;
    };
    Ok(current_block_count)
}

fn block_count_sans_ansi_codes(strings: &[ANSIString<'_>]) -> u16 {
    strings.iter().map(|s| s.width() as u16).sum()
}

fn draw_progress_bar(p: &Value, style: Style, mut blocks_available: u16, colored: bool, buf: &mut Vec<ANSIString<'_>>) {
    let mut brush = color::Brush::new(colored);
    let styled_brush = brush.style(style);

    blocks_available = blocks_available.saturating_sub(3); // account for…I don't really know it's magic
    buf.push(" [".into());
    match p.fraction() {
        Some(mut fraction) => {
            fraction = fraction.min(1.0);
            blocks_available = blocks_available.saturating_sub(1); // account for '>' apparently
            let progress_blocks = (blocks_available as f32 * fraction).floor() as usize;
            buf.push(styled_brush.paint(format!("{:=<width$}", "", width = progress_blocks)));
            buf.push(styled_brush.paint(">"));
            buf.push(styled_brush.style(style.dimmed()).paint(format!(
                "{:-<width$}",
                "",
                width = (blocks_available - progress_blocks as u16) as usize
            )));
        }
        None => {
            const CHARS: [char; 6] = ['=', '=', '=', ' ', ' ', ' '];
            buf.push(
                styled_brush.paint(
                    (p.step.load(Ordering::SeqCst)..std::usize::MAX)
                        .take(blocks_available as usize)
                        .map(|idx| CHARS[idx % CHARS.len()])
                        .rev()
                        .collect::<String>(),
                ),
            );
        }
    }
    buf.push("]".into());
}

fn progress_style(p: &Value) -> Style {
    use crate::progress::State::*;
    match p.state {
        Running => if let Some(fraction) = p.fraction() {
            if fraction > 0.8 {
                Color::Green
            } else {
                Color::Yellow
            }
        } else {
            Color::White
        }
        .normal(),
        Halted(_, _) => Color::Red.dimmed(),
        Blocked(_, _) => Color::Red.normal(),
    }
}

fn format_progress<'a>(
    key: &progress::Key,
    value: &'a progress::Task,
    column_count: u16,
    colored: bool,
    midpoint: Option<u16>,
    throughput: Option<unit::display::Throughput>,
    buf: &mut Vec<ANSIString<'a>>,
) -> Option<u16> {
    let mut brush = color::Brush::new(colored);
    buf.clear();

    buf.push(Style::new().paint(format!("{:>level$}", "", level = key.level() as usize)));
    match value.progress.as_ref() {
        Some(progress) => {
            let style = progress_style(progress);
            buf.push(brush.style(Color::Cyan.bold()).paint(&value.name));
            buf.push(" ".into());

            let pre_unit = buf.len();
            let values_brush = brush.style(Style::new().bold().dimmed());
            match progress.unit.as_ref() {
                Some(unit) => {
                    let mut display = unit.display(progress.step.load(Ordering::SeqCst), progress.done_at, throughput);
                    buf.push(values_brush.paint(display.values().to_string()));
                    buf.push(" ".into());
                    buf.push(display.unit().to_string().into());
                }
                None => {
                    buf.push(values_brush.paint(match progress.done_at {
                        Some(done_at) => format!("{}/{}", progress.step.load(Ordering::SeqCst), done_at),
                        None => format!("{}", progress.step.load(Ordering::SeqCst)),
                    }));
                }
            }
            let desired_midpoint = block_count_sans_ansi_codes(buf.as_slice());
            let actual_midpoint = if let Some(midpoint) = midpoint {
                let padding = midpoint.saturating_sub(desired_midpoint);
                if padding > 0 {
                    buf.insert(pre_unit, " ".repeat(padding as usize).into());
                }
                block_count_sans_ansi_codes(buf.as_slice())
            } else {
                desired_midpoint
            };
            let blocks_left = column_count.saturating_sub(actual_midpoint);
            if blocks_left > 0 {
                draw_progress_bar(progress, style, blocks_left, colored, buf);
            }
            Some(desired_midpoint)
        }
        None => {
            // headline only - FIXME: would have to truncate it if it is too long for the line…
            buf.push(brush.style(Color::White.bold()).paint(&value.name));
            None
        }
    }
}
