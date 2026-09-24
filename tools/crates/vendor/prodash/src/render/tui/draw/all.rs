use std::time::Duration;

use tui::{
    buffer::Buffer,
    layout::Rect,
    style::{Modifier, Style},
    text::Span,
    widgets::{Block, Borders, Widget},
};

use crate::{
    messages::Message,
    progress::{Key, Task},
    render::tui::{
        draw,
        utils::{block_width, rect},
        InterruptDrawInfo, Line,
    },
    Throughput,
};

#[derive(Default)]
pub struct State {
    pub title: String,
    pub task_offset: u16,
    pub message_offset: u16,
    pub hide_messages: bool,
    pub messages_fullscreen: bool,
    pub user_provided_window_size: Option<Rect>,
    pub duration_per_frame: Duration,
    pub information: Vec<Line>,
    pub hide_info: bool,
    pub maximize_info: bool,
    pub last_tree_column_width: Option<u16>,
    pub next_tree_column_width: Option<u16>,
    pub throughput: Option<Throughput>,
}

pub(crate) fn all(
    state: &mut State,
    interrupt_mode: InterruptDrawInfo,
    entries: &[(Key, Task)],
    messages: &[Message],
    bound: Rect,
    buf: &mut Buffer,
) {
    let (bound, info_pane) = compute_info_bound(
        bound,
        if state.hide_info { &[] } else { &state.information },
        state.maximize_info,
    );
    let bold = Style::default().add_modifier(Modifier::BOLD);
    let window = Block::default()
        .title(Span::styled(state.title.as_str(), bold))
        .borders(Borders::ALL);
    let inner_area = window.inner(bound);
    window.render(bound, buf);
    if bound.width < 4 || bound.height < 4 {
        return;
    }

    let border_width = 1;
    draw::progress::headline(
        entries,
        interrupt_mode,
        state.duration_per_frame,
        buf,
        rect::offset_x(
            Rect {
                height: 1,
                width: bound.width.saturating_sub(border_width),
                ..bound
            },
            block_width(&state.title) + (border_width * 2),
        ),
    );

    let (progress_pane, messages_pane) = compute_pane_bounds(
        if state.hide_messages { &[] } else { messages },
        inner_area,
        state.messages_fullscreen,
    );

    draw::progress::pane(entries, progress_pane, buf, state);
    if let Some(messages_pane) = messages_pane {
        draw::messages::pane(
            messages,
            messages_pane,
            Rect {
                width: messages_pane.width + 2,
                ..rect::line_bound(bound, bound.height.saturating_sub(1) as usize)
            },
            &mut state.message_offset,
            buf,
        );
    }

    if let Some(info_pane) = info_pane {
        draw::information::pane(&state.information, info_pane, buf);
    }
}

fn compute_pane_bounds(messages: &[Message], inner: Rect, messages_fullscreen: bool) -> (Rect, Option<Rect>) {
    if messages.is_empty() {
        (inner, None)
    } else {
        let (task_percent, messages_percent) = if messages_fullscreen { (0.1, 0.9) } else { (0.75, 0.25) };
        let tasks_height: u16 = (inner.height as f32 * task_percent).ceil() as u16;
        let messages_height: u16 = (inner.height as f32 * messages_percent).floor() as u16;
        if messages_height < 2 {
            (inner, None)
        } else {
            let messages_title = 1u16;
            let new_messages_height = messages_height.min((messages.len() + messages_title as usize) as u16);
            let tasks_height = tasks_height.saturating_add(messages_height - new_messages_height);
            let messages_height = new_messages_height;
            (
                Rect {
                    height: tasks_height,
                    ..inner
                },
                Some(rect::intersect(
                    Rect {
                        y: tasks_height + messages_title,
                        height: messages_height,
                        ..inner
                    },
                    inner,
                )),
            )
        }
    }
}

fn compute_info_bound(bound: Rect, info: &[Line], maximize: bool) -> (Rect, Option<Rect>) {
    if info.is_empty() {
        return (bound, None);
    }
    let margin = 1;
    let max_line_width = info.iter().fold(0, |state, l| {
        state.max(
            block_width(match l {
                Line::Text(s) | Line::Title(s) => s,
            }) + margin * 2,
        )
    });
    let pane_width = if maximize {
        bound.width.saturating_sub(8).min(max_line_width)
    } else {
        (bound.width / 3).min(max_line_width)
    };

    if pane_width < max_line_width / 3 {
        return (bound, None);
    }

    (
        Rect {
            width: bound.width.saturating_sub(pane_width),
            ..bound
        },
        Some(rect::snap_to_right(bound, pane_width)),
    )
}
