use std::time::SystemTime;

use tui::{
    buffer::Buffer,
    layout::Rect,
    style::{Color, Modifier, Style},
    text::Span,
    widgets::{Block, Borders, Widget},
};
use unicode_width::UnicodeWidthStr;

use crate::{
    messages::{Message, MessageLevel},
    render::tui::utils::{block_width, draw_text_with_ellipsis_nowrap, rect, sanitize_offset, VERTICAL_LINE},
    time::{format_time_for_messages, DATE_TIME_HMS},
};

pub fn pane(messages: &[Message], bound: Rect, overflow_bound: Rect, offset: &mut u16, buf: &mut Buffer) {
    let bold = Style::default().add_modifier(Modifier::BOLD);
    let block = Block::default()
        .title(Span::styled("Messages", bold))
        .borders(Borders::TOP);
    let inner_bound = block.inner(bound);
    block.render(bound, buf);
    let help_text = " ⨯ = `| ▢ = ~ ";
    draw_text_with_ellipsis_nowrap(rect::snap_to_right(bound, block_width(help_text)), buf, help_text, bold);

    let bound = inner_bound;
    *offset = sanitize_offset(*offset, messages.len(), bound.height);
    let max_origin_width = messages
        .iter()
        .rev()
        .skip(*offset as usize)
        .take(bound.height as usize)
        .fold(0, |state, message| state.max(block_width(&message.origin)));
    for (
        line,
        Message {
            time,
            message,
            level,
            origin,
        },
    ) in messages
        .iter()
        .rev()
        .skip(*offset as usize)
        .take(bound.height as usize)
        .enumerate()
    {
        let line_bound = rect::line_bound(bound, line);
        let (time_bound, level_bound, origin_bound, message_bound) = compute_bounds(line_bound, max_origin_width);
        if let Some(time_bound) = time_bound {
            draw_text_with_ellipsis_nowrap(time_bound, buf, format_time_column(time), None);
        }
        if let Some(level_bound) = level_bound {
            draw_text_with_ellipsis_nowrap(
                level_bound,
                buf,
                format_level_column(*level),
                Some(level_to_style(*level)),
            );
            draw_text_with_ellipsis_nowrap(rect::offset_x(level_bound, LEVEL_TEXT_WIDTH), buf, VERTICAL_LINE, None);
        }
        if let Some(origin_bound) = origin_bound {
            draw_text_with_ellipsis_nowrap(origin_bound, buf, origin, None);
            draw_text_with_ellipsis_nowrap(rect::offset_x(origin_bound, max_origin_width), buf, "→", None);
        }
        draw_text_with_ellipsis_nowrap(message_bound, buf, message, None);
    }

    if (bound.height as usize) < messages.len().saturating_sub(*offset as usize)
        || (*offset).min(messages.len() as u16) > 0
    {
        let messages_below = messages
            .len()
            .saturating_sub(bound.height.saturating_add(*offset) as usize);
        let messages_skipped = (*offset).min(messages.len() as u16);
        draw_text_with_ellipsis_nowrap(
            rect::offset_x(overflow_bound, 1),
            buf,
            format!("… {} skipped and {} more", messages_skipped, messages_below),
            bold,
        );
        let help_text = " ⇊ = D|↓ = J|⇈ = U|↑ = K ┘";
        draw_text_with_ellipsis_nowrap(
            rect::snap_to_right(overflow_bound, block_width(help_text)),
            buf,
            help_text,
            bold,
        );
    }
}

const LEVEL_TEXT_WIDTH: u16 = 4;
fn format_level_column(level: MessageLevel) -> &'static str {
    use MessageLevel::*;
    match level {
        Info => "info",
        Failure => "fail",
        Success => "done",
    }
}

fn level_to_style(level: MessageLevel) -> Style {
    use MessageLevel::*;
    Style::default()
        .fg(Color::Black)
        .add_modifier(Modifier::BOLD)
        .bg(match level {
            Info => Color::White,
            Failure => Color::Red,
            Success => Color::Green,
        })
}

fn format_time_column(time: &SystemTime) -> String {
    format!("{}{}", format_time_for_messages(*time), VERTICAL_LINE)
}

fn compute_bounds(line: Rect, max_origin_width: u16) -> (Option<Rect>, Option<Rect>, Option<Rect>, Rect) {
    let vertical_line_width = VERTICAL_LINE.width() as u16;
    let mythical_offset_we_should_not_need = 1;

    let time_bound = Rect {
        width: DATE_TIME_HMS as u16 + vertical_line_width,
        ..line
    };

    let mut cursor = time_bound.width + mythical_offset_we_should_not_need;
    let level_bound = Rect {
        x: cursor,
        width: LEVEL_TEXT_WIDTH + vertical_line_width,
        ..line
    };
    cursor += level_bound.width;

    let origin_bound = Rect {
        x: cursor,
        width: max_origin_width + vertical_line_width,
        ..line
    };
    cursor += origin_bound.width;

    let message_bound = rect::intersect(rect::offset_x(line, cursor), line);
    if message_bound.width < 30 {
        return (None, None, None, line);
    }
    (Some(time_bound), Some(level_bound), Some(origin_bound), message_bound)
}
