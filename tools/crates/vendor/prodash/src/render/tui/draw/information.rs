use tui::{
    buffer::Buffer,
    layout::Rect,
    style::{Modifier, Style},
    text::Span,
    widgets::{Block, Borders, Widget},
};

use crate::render::tui::{
    utils::{block_width, draw_text_with_ellipsis_nowrap, rect},
    Line,
};

pub fn pane(lines: &[Line], bound: Rect, buf: &mut Buffer) {
    let bold = Style::default().add_modifier(Modifier::BOLD);
    let block = Block::default()
        .title(Span::styled("Information", bold))
        .borders(Borders::TOP | Borders::BOTTOM);
    let inner_bound = block.inner(bound);
    block.render(bound, buf);

    let help_text = " ⨯ = [ | ▢ = { ";
    draw_text_with_ellipsis_nowrap(rect::snap_to_right(bound, block_width(help_text)), buf, help_text, bold);

    let bound = Rect {
        width: inner_bound.width.saturating_sub(1),
        ..inner_bound
    };
    let mut offset = 0;
    for (line, info) in lines.windows(2).enumerate() {
        let (info, next_info) = (&info[0], &info[1]);
        let line = line + offset;
        if line >= bound.height as usize {
            break;
        }
        let line_bound = rect::line_bound(bound, line);
        match info {
            Line::Title(text) => {
                let blocks_drawn = draw_text_with_ellipsis_nowrap(line_bound, buf, text, bold);
                let lines_rect = rect::offset_x(line_bound, blocks_drawn + 1);
                for x in lines_rect.left()..lines_rect.right() {
                    buf.get_mut(x, lines_rect.y).set_symbol("─");
                }
                offset += 1;
            }
            Line::Text(text) => {
                draw_text_with_ellipsis_nowrap(rect::offset_x(line_bound, 1), buf, text, None);
            }
        };
        if let Line::Title(_) = next_info {
            offset += 1;
        }
    }

    if let Some(Line::Text(text)) = lines.last() {
        let line = lines.len().saturating_sub(1) + offset;
        if line < bound.height as usize {
            draw_text_with_ellipsis_nowrap(rect::offset_x(rect::line_bound(bound, line), 1), buf, text, bold);
        }
    }
}
