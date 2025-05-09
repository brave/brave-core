use std::collections::BTreeMap;
use std::iter;

use group::ff::Field;

use super::FailureLocation;
use crate::{
    dev::{metadata, util},
    plonk::{Any, Expression},
};

fn padded(p: char, width: usize, text: &str) -> String {
    let pad = width - text.len();
    format!(
        "{}{}{}",
        iter::repeat(p).take(pad - pad / 2).collect::<String>(),
        text,
        iter::repeat(p).take(pad / 2).collect::<String>(),
    )
}

/// Renders a cell layout around a given failure location.
///
/// `highlight_row` is called at the end of each row, with the offset of the active row
/// (if `location` is in a region), and the rotation of the current row relative to the
/// active row.
pub(super) fn render_cell_layout(
    prefix: &str,
    location: &FailureLocation,
    columns: &BTreeMap<metadata::Column, usize>,
    layout: &BTreeMap<i32, BTreeMap<metadata::Column, String>>,
    highlight_row: impl Fn(Option<i32>, i32),
) {
    let col_width = |cells: usize| cells.to_string().len() + 3;

    // If we are in a region, show rows at offsets relative to it. Otherwise, just show
    // the rotations directly.
    let offset = match location {
        FailureLocation::InRegion { region, offset } => {
            eprintln!("{}Cell layout in region '{}':", prefix, region.name);
            eprint!("{}  | Offset |", prefix);
            Some(*offset as i32)
        }
        FailureLocation::OutsideRegion { row } => {
            eprintln!("{}Cell layout at row {}:", prefix, row);
            eprint!("{}  |Rotation|", prefix);
            None
        }
    };

    // Print the assigned cells, and their region offset or rotation.
    for (column, cells) in columns {
        let width = col_width(*cells);
        eprint!(
            "{}|",
            padded(
                ' ',
                width,
                &format!(
                    "{}{}",
                    match column.column_type {
                        Any::Advice => "A",
                        Any::Fixed => "F",
                        Any::Instance => "I",
                    },
                    column.index,
                )
            )
        );
    }
    eprintln!();
    eprint!("{}  +--------+", prefix);
    for cells in columns.values() {
        eprint!("{}+", padded('-', col_width(*cells), ""));
    }
    eprintln!();
    for (rotation, row) in layout {
        eprint!(
            "{}  |{}|",
            prefix,
            padded(' ', 8, &(offset.unwrap_or(0) + rotation).to_string())
        );
        for (col, cells) in columns {
            let width = col_width(*cells);
            eprint!(
                "{}|",
                padded(
                    ' ',
                    width,
                    row.get(col).map(|s| s.as_str()).unwrap_or_default()
                )
            );
        }
        highlight_row(offset, *rotation);
        eprintln!();
    }
}

pub(super) fn expression_to_string<F: Field>(
    expr: &Expression<F>,
    layout: &BTreeMap<i32, BTreeMap<metadata::Column, String>>,
) -> String {
    expr.evaluate(
        &util::format_value,
        &|_| panic!("virtual selectors are removed during optimization"),
        &|query| {
            if let Some(label) = layout
                .get(&query.rotation.0)
                .and_then(|row| row.get(&(Any::Fixed, query.column_index).into()))
            {
                label.clone()
            } else if query.rotation.0 == 0 {
                // This is most likely a merged selector
                format!("S{}", query.index)
            } else {
                // No idea how we'd get here...
                format!("F{}@{}", query.column_index, query.rotation.0)
            }
        },
        &|query| {
            layout
                .get(&query.rotation.0)
                .unwrap()
                .get(&(Any::Advice, query.column_index).into())
                .unwrap()
                .clone()
        },
        &|query| {
            layout
                .get(&query.rotation.0)
                .unwrap()
                .get(&(Any::Instance, query.column_index).into())
                .unwrap()
                .clone()
        },
        &|a| {
            if a.contains(' ') {
                format!("-({})", a)
            } else {
                format!("-{}", a)
            }
        },
        &|a, b| {
            if let Some(b) = b.strip_prefix('-') {
                format!("{} - {}", a, b)
            } else {
                format!("{} + {}", a, b)
            }
        },
        &|a, b| match (a.contains(' '), b.contains(' ')) {
            (false, false) => format!("{} * {}", a, b),
            (false, true) => format!("{} * ({})", a, b),
            (true, false) => format!("({}) * {}", a, b),
            (true, true) => format!("({}) * ({})", a, b),
        },
        &|a, s| {
            if a.contains(' ') {
                format!("({}) * {}", a, util::format_value(s))
            } else {
                format!("{} * {}", a, util::format_value(s))
            }
        },
    )
}
