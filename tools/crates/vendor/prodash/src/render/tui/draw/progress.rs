use std::{
    fmt,
    sync::atomic::Ordering,
    time::{Duration, SystemTime},
};

use humantime::format_duration;
use tui::{
    buffer::Buffer,
    layout::Rect,
    style::{Color, Modifier, Style},
};
use tui_react::fill_background;

use crate::{
    progress::{self, Key, Step, Task, Value},
    render::tui::{
        draw::State,
        utils::{
            block_width, draw_text_nowrap_fn, draw_text_with_ellipsis_nowrap, rect, sanitize_offset,
            GraphemeCountWriter, VERTICAL_LINE,
        },
        InterruptDrawInfo,
    },
    time::format_now_datetime_seconds,
    unit, Throughput,
};

const MIN_TREE_WIDTH: u16 = 20;

pub fn pane(entries: &[(Key, progress::Task)], mut bound: Rect, buf: &mut Buffer, state: &mut State) {
    state.task_offset = sanitize_offset(state.task_offset, entries.len(), bound.height);
    let needs_overflow_line =
        if entries.len() > bound.height as usize || (state.task_offset).min(entries.len() as u16) > 0 {
            bound.height = bound.height.saturating_sub(1);
            true
        } else {
            false
        };
    state.task_offset = sanitize_offset(state.task_offset, entries.len(), bound.height);

    if entries.is_empty() {
        return;
    }

    let initial_column_width = bound.width / 3;
    let desired_max_tree_draw_width = *state.next_tree_column_width.as_ref().unwrap_or(&initial_column_width);
    {
        if initial_column_width >= MIN_TREE_WIDTH {
            let tree_bound = Rect {
                width: desired_max_tree_draw_width,
                ..bound
            };
            let computed = draw_tree(entries, buf, tree_bound, state.task_offset);
            state.last_tree_column_width = Some(computed);
        } else {
            state.last_tree_column_width = Some(0);
        };
    }

    {
        if let Some(tp) = state.throughput.as_mut() {
            tp.update_elapsed();
        }

        let progress_area = rect::offset_x(bound, desired_max_tree_draw_width);
        draw_progress(
            entries,
            buf,
            progress_area,
            state.task_offset,
            state.throughput.as_mut(),
        );

        if let Some(tp) = state.throughput.as_mut() {
            tp.reconcile(entries);
        }
    }

    if needs_overflow_line {
        let overflow_rect = Rect {
            y: bound.height + 1,
            height: 1,
            ..bound
        };
        draw_overflow(
            entries,
            buf,
            overflow_rect,
            desired_max_tree_draw_width,
            bound.height,
            state.task_offset,
        );
    }
}

pub(crate) fn headline(
    entries: &[(Key, Task)],
    interrupt_mode: InterruptDrawInfo,
    duration_per_frame: Duration,
    buf: &mut Buffer,
    bound: Rect,
) {
    let (num_running_tasks, num_blocked_tasks, num_groups) = entries.iter().fold(
        (0, 0, 0),
        |(mut running, mut blocked, mut groups), (_key, Task { progress, .. })| {
            match progress.as_ref().map(|p| p.state) {
                Some(progress::State::Running) => running += 1,
                Some(progress::State::Blocked(_, _)) | Some(progress::State::Halted(_, _)) => blocked += 1,
                None => groups += 1,
            }
            (running, blocked, groups)
        },
    );
    let text = format!(
        " {} {} {:3} running + {:3} blocked + {:3} groups = {} ",
        match interrupt_mode {
            InterruptDrawInfo::Instantly => "'q' or CTRL+c to quit",
            InterruptDrawInfo::Deferred(interrupt_requested) => {
                if interrupt_requested {
                    "interrupt requested - please wait"
                } else {
                    "cannot interrupt current operation"
                }
            }
        },
        if duration_per_frame > Duration::from_secs(1) {
            format!(
                " Every {}s → {}",
                duration_per_frame.as_secs(),
                format_now_datetime_seconds()
            )
        } else {
            "".into()
        },
        num_running_tasks,
        num_blocked_tasks,
        num_groups,
        entries.len()
    );

    let bold = Style::default().add_modifier(Modifier::BOLD);
    draw_text_with_ellipsis_nowrap(rect::snap_to_right(bound, block_width(&text) + 1), buf, text, bold);
}

struct ProgressFormat<'a>(&'a Option<Value>, u16, Option<unit::display::Throughput>);

impl<'a> fmt::Display for ProgressFormat<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.0 {
            Some(p) => match p.unit.as_ref() {
                Some(unit) => write!(
                    f,
                    "{}",
                    unit.display(p.step.load(Ordering::SeqCst), p.done_at, self.2.clone())
                ),
                None => match p.done_at {
                    Some(done_at) => write!(f, "{}/{}", p.step.load(Ordering::SeqCst), done_at),
                    None => write!(f, "{}", p.step.load(Ordering::SeqCst)),
                },
            },
            None => write!(f, "{:─<width$}", '─', width = self.1 as usize),
        }
    }
}

fn has_child(entries: &[(Key, Task)], index: usize) -> bool {
    entries
        .get(index + 1)
        .and_then(|(other_key, other_val)| {
            entries.get(index).map(|(cur_key, _)| {
                cur_key.shares_parent_with(other_key, cur_key.level()) && other_val.progress.is_some()
            })
        })
        .unwrap_or(false)
}

pub fn draw_progress(
    entries: &[(Key, Task)],
    buf: &mut Buffer,
    bound: Rect,
    offset: u16,
    mut throughput: Option<&mut Throughput>,
) {
    let title_spacing = 2u16 + 1; // 2 on the left, 1 on the right
    let max_progress_label_width = entries
        .iter()
        .skip(offset as usize)
        .take(bound.height as usize)
        .map(|(_, Task { progress, .. })| progress)
        .fold(0, |state, progress| match progress {
            progress @ Some(_) => {
                use std::io::Write;
                let mut w = GraphemeCountWriter::default();
                write!(w, "{}", ProgressFormat(progress, 0, None)).expect("never fails");
                state.max(w.0)
            }
            None => state,
        });

    for (
        line,
        (
            entry_index,
            (
                key,
                Task {
                    progress,
                    name: title,
                    id: _,
                },
            ),
        ),
    ) in entries
        .iter()
        .enumerate()
        .skip(offset as usize)
        .take(bound.height as usize)
        .enumerate()
    {
        let throughput = throughput
            .as_mut()
            .and_then(|tp| tp.update_and_get(key, progress.as_ref()));
        let line_bound = rect::line_bound(bound, line);
        let progress_text = format!(
            " {progress}",
            progress = ProgressFormat(
                progress,
                if has_child(entries, entry_index) {
                    bound.width.saturating_sub(title_spacing)
                } else {
                    0
                },
                throughput
            )
        );

        draw_text_with_ellipsis_nowrap(line_bound, buf, VERTICAL_LINE, None);

        let tree_prefix = level_prefix(entries, entry_index);
        let progress_rect = rect::offset_x(line_bound, block_width(&tree_prefix));
        draw_text_with_ellipsis_nowrap(line_bound, buf, tree_prefix, None);
        match progress
            .as_ref()
            .map(|p| (p.fraction(), p.state, p.step.load(Ordering::SeqCst)))
        {
            Some((Some(fraction), state, _step)) => {
                let mut progress_text = progress_text;
                add_block_eta(state, &mut progress_text);
                let (bound, style) = draw_progress_bar_fn(buf, progress_rect, fraction, |fraction| match state {
                    progress::State::Blocked(_, _) => Color::Red,
                    progress::State::Halted(_, _) => Color::LightRed,
                    progress::State::Running => {
                        if fraction >= 0.8 {
                            Color::Green
                        } else {
                            Color::Yellow
                        }
                    }
                });
                let style_fn = move |_t: &str, x: u16, _y: u16| {
                    if x < bound.right() {
                        style
                    } else {
                        Style::default()
                    }
                };
                draw_text_nowrap_fn(progress_rect, buf, progress_text, style_fn);
            }
            Some((None, state, step)) => {
                let mut progress_text = progress_text;
                add_block_eta(state, &mut progress_text);
                draw_text_with_ellipsis_nowrap(progress_rect, buf, progress_text, None);
                let bar_rect = rect::offset_x(line_bound, max_progress_label_width as u16);
                draw_spinner(
                    buf,
                    bar_rect,
                    step,
                    line,
                    match state {
                        progress::State::Blocked(_, _) => Color::Red,
                        progress::State::Halted(_, _) => Color::LightRed,
                        progress::State::Running => Color::White,
                    },
                );
            }
            None => {
                let bold = Style::default().add_modifier(Modifier::BOLD);
                draw_text_nowrap_fn(progress_rect, buf, progress_text, |_, _, _| Style::default());
                draw_text_with_ellipsis_nowrap(progress_rect, buf, format!(" {} ", title), bold);
            }
        }
    }
}

fn add_block_eta(state: progress::State, progress_text: &mut String) {
    match state {
        progress::State::Blocked(reason, maybe_eta) | progress::State::Halted(reason, maybe_eta) => {
            progress_text.push_str(" [");
            progress_text.push_str(reason);
            progress_text.push(']');
            if let Some(eta) = maybe_eta {
                let now = SystemTime::now();
                if eta > now {
                    use std::fmt::Write;
                    write!(
                        progress_text,
                        " → {} to {}",
                        format_duration(eta.duration_since(now).expect("computation to work")),
                        if let progress::State::Blocked(_, _) = state {
                            "unblock"
                        } else {
                            "continue"
                        }
                    )
                    .expect("in-memory writes never fail");
                }
            }
        }
        progress::State::Running => {}
    }
}

fn draw_spinner(buf: &mut Buffer, bound: Rect, step: Step, seed: usize, color: Color) {
    if bound.width == 0 {
        return;
    }
    let x = bound.x + ((step + seed) % bound.width as usize) as u16;
    let width = 5;
    let bound = rect::intersect(Rect { x, width, ..bound }, bound);
    tui_react::fill_background(bound, buf, color);
}

fn draw_progress_bar_fn(
    buf: &mut Buffer,
    bound: Rect,
    fraction: f32,
    style: impl FnOnce(f32) -> Color,
) -> (Rect, Style) {
    if bound.width == 0 {
        return (Rect::default(), Style::default());
    }
    let mut fractional_progress_rect = Rect {
        width: ((bound.width as f32 * fraction).floor() as u16).min(bound.width),
        ..bound
    };
    let color = style(fraction);
    for y in fractional_progress_rect.top()..fractional_progress_rect.bottom() {
        for x in fractional_progress_rect.left()..fractional_progress_rect.right() {
            let cell = buf.get_mut(x, y);
            cell.set_fg(color);
            cell.set_symbol(tui::symbols::block::FULL);
        }
    }
    if fractional_progress_rect.width < bound.width {
        static BLOCK_SECTIONS: [&str; 9] = [
            " ",
            tui::symbols::block::ONE_EIGHTH,
            tui::symbols::block::ONE_QUARTER,
            tui::symbols::block::THREE_EIGHTHS,
            tui::symbols::block::HALF,
            tui::symbols::block::FIVE_EIGHTHS,
            tui::symbols::block::THREE_QUARTERS,
            tui::symbols::block::SEVEN_EIGHTHS,
            tui::symbols::block::FULL,
        ];
        // Get the index based on how filled the remaining part is
        let index = ((((bound.width as f32 * fraction) - fractional_progress_rect.width as f32) * 8f32).round()
            as usize)
            % BLOCK_SECTIONS.len();
        let cell = buf.get_mut(fractional_progress_rect.right(), bound.y);
        cell.set_symbol(BLOCK_SECTIONS[index]);
        cell.set_fg(color);
        fractional_progress_rect.width += 1;
    }
    (fractional_progress_rect, Style::default().bg(color).fg(Color::Black))
}

pub fn draw_tree(entries: &[(Key, Task)], buf: &mut Buffer, bound: Rect, offset: u16) -> u16 {
    let mut max_prefix_len = 0;
    for (line, (entry_index, entry)) in entries
        .iter()
        .enumerate()
        .skip(offset as usize)
        .take(bound.height as usize)
        .enumerate()
    {
        let mut line_bound = rect::line_bound(bound, line);
        line_bound.x = line_bound.x.saturating_sub(1);
        line_bound.width = line_bound.width.saturating_sub(1);
        let tree_prefix = format!("{} {} ", level_prefix(entries, entry_index), entry.1.name);
        max_prefix_len = max_prefix_len.max(block_width(&tree_prefix));

        let style = if entry.1.progress.is_none() {
            Style::default().add_modifier(Modifier::BOLD).into()
        } else {
            None
        };
        draw_text_with_ellipsis_nowrap(line_bound, buf, tree_prefix, style);
    }
    max_prefix_len
}

fn level_prefix(entries: &[(Key, Task)], entry_index: usize) -> String {
    let adj = Key::adjacency(entries, entry_index);
    let key = entries[entry_index].0;
    let key_level = key.level();
    let is_orphan = adj.level() != key_level;
    let mut buf = String::with_capacity(key_level as usize);
    for level in 1..=key_level {
        use crate::progress::key::SiblingLocation::*;
        let is_child_level = level == key_level;
        if level != 1 {
            buf.push(' ');
        }
        if level == 1 && is_child_level {
            buf.push(match adj[level] {
                AboveAndBelow | Above => '├',
                NotFound | Below => '│',
            });
        } else {
            let c = if is_child_level {
                match adj[level] {
                    NotFound => {
                        if is_orphan {
                            ' '
                        } else {
                            '·'
                        }
                    }
                    Above => '└',
                    Below => '┌',
                    AboveAndBelow => '├',
                }
            } else {
                match adj[level] {
                    NotFound => {
                        if level == 1 {
                            '│'
                        } else if is_orphan {
                            '·'
                        } else {
                            ' '
                        }
                    }
                    Above => '└',
                    Below => '┌',
                    AboveAndBelow => '│',
                }
            };
            buf.push(c)
        }
    }
    buf
}

pub fn draw_overflow(
    entries: &[(Key, Task)],
    buf: &mut Buffer,
    bound: Rect,
    label_offset: u16,
    num_entries_on_display: u16,
    offset: u16,
) {
    let (count, mut progress_fraction) = entries
        .iter()
        .take(offset as usize)
        .chain(entries.iter().skip((offset + num_entries_on_display) as usize))
        .fold((0usize, 0f32), |(count, progress_fraction), (_key, value)| {
            let progress = value.progress.as_ref().and_then(|p| p.fraction()).unwrap_or_default();
            (count + 1, progress_fraction + progress)
        });
    progress_fraction /= count as f32;
    let label = format!(
        "{} …{} skipped and {} more",
        if label_offset == 0 { "" } else { VERTICAL_LINE },
        offset,
        entries
            .len()
            .saturating_sub((offset + num_entries_on_display + 1) as usize)
    );
    let (progress_rect, style) = draw_progress_bar_fn(buf, bound, progress_fraction, |_| Color::Green);

    let bg_color = Color::Red;
    fill_background(rect::offset_x(bound, progress_rect.right() - 1), buf, bg_color);
    let color_text_according_to_progress = move |_g: &str, x: u16, _y: u16| {
        if x < progress_rect.right() {
            style
        } else {
            style.bg(bg_color)
        }
    };
    draw_text_nowrap_fn(
        rect::offset_x(bound, label_offset),
        buf,
        label,
        color_text_according_to_progress,
    );
    let help_text = "⇊ = d|↓ = j|⇈ = u|↑ = k ";
    draw_text_nowrap_fn(
        rect::snap_to_right(bound, block_width(help_text)),
        buf,
        help_text,
        color_text_according_to_progress,
    );
}
