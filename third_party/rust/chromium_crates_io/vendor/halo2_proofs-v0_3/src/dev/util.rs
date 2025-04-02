use std::collections::BTreeMap;

use group::ff::Field;

use super::{metadata, CellValue, InstanceValue, Value};
use crate::{
    plonk::{
        AdviceQuery, Any, Column, ColumnType, Expression, FixedQuery, Gate, InstanceQuery,
        VirtualCell,
    },
    poly::Rotation,
};

pub(crate) struct AnyQuery {
    /// Query index
    pub index: usize,
    /// Column type
    pub column_type: Any,
    /// Column index
    pub column_index: usize,
    /// Rotation of this query
    pub rotation: Rotation,
}

impl From<FixedQuery> for AnyQuery {
    fn from(query: FixedQuery) -> Self {
        Self {
            index: query.index,
            column_type: Any::Fixed,
            column_index: query.column_index,
            rotation: query.rotation,
        }
    }
}

impl From<AdviceQuery> for AnyQuery {
    fn from(query: AdviceQuery) -> Self {
        Self {
            index: query.index,
            column_type: Any::Advice,
            column_index: query.column_index,
            rotation: query.rotation,
        }
    }
}

impl From<InstanceQuery> for AnyQuery {
    fn from(query: InstanceQuery) -> Self {
        Self {
            index: query.index,
            column_type: Any::Instance,
            column_index: query.column_index,
            rotation: query.rotation,
        }
    }
}

pub(super) fn format_value<F: Field>(v: F) -> String {
    if v.is_zero_vartime() {
        "0".into()
    } else if v == F::ONE {
        "1".into()
    } else if v == -F::ONE {
        "-1".into()
    } else {
        // Format value as hex.
        let s = format!("{:?}", v);
        // Remove leading zeroes.
        let s = s.strip_prefix("0x").unwrap();
        let s = s.trim_start_matches('0');
        format!("0x{}", s)
    }
}

pub(super) fn load<'a, F: Field, T: ColumnType, Q: Into<AnyQuery> + Copy>(
    n: i32,
    row: i32,
    queries: &'a [(Column<T>, Rotation)],
    cells: &'a [Vec<CellValue<F>>],
) -> impl Fn(Q) -> Value<F> + 'a {
    move |query| {
        let (column, at) = &queries[query.into().index];
        let resolved_row = (row + at.0) % n;
        cells[column.index()][resolved_row as usize].into()
    }
}

pub(super) fn load_instance<'a, F: Field, T: ColumnType, Q: Into<AnyQuery> + Copy>(
    n: i32,
    row: i32,
    queries: &'a [(Column<T>, Rotation)],
    cells: &'a [Vec<InstanceValue<F>>],
) -> impl Fn(Q) -> Value<F> + 'a {
    move |query| {
        let (column, at) = &queries[query.into().index];
        let resolved_row = (row + at.0) % n;
        let cell = &cells[column.index()][resolved_row as usize];
        Value::Real(cell.value())
    }
}

fn cell_value<'a, F: Field, Q: Into<AnyQuery> + Copy>(
    virtual_cells: &'a [VirtualCell],
    load: impl Fn(Q) -> Value<F> + 'a,
) -> impl Fn(Q) -> BTreeMap<metadata::VirtualCell, String> + 'a {
    move |query| {
        let AnyQuery {
            column_type,
            column_index,
            rotation,
            ..
        } = query.into();
        virtual_cells
            .iter()
            .find(|c| {
                c.column.column_type() == &column_type
                    && c.column.index() == column_index
                    && c.rotation == rotation
            })
            // None indicates a selector, which we don't bother showing.
            .map(|cell| {
                (
                    cell.clone().into(),
                    match load(query) {
                        Value::Real(v) => format_value(v),
                        Value::Poison => unreachable!(),
                    },
                )
            })
            .into_iter()
            .collect()
    }
}

pub(super) fn cell_values<'a, F: Field>(
    gate: &Gate<F>,
    poly: &Expression<F>,
    load_fixed: impl Fn(FixedQuery) -> Value<F> + 'a,
    load_advice: impl Fn(AdviceQuery) -> Value<F> + 'a,
    load_instance: impl Fn(InstanceQuery) -> Value<F> + 'a,
) -> Vec<(metadata::VirtualCell, String)> {
    let virtual_cells = gate.queried_cells();
    let cell_values = poly.evaluate(
        &|_| BTreeMap::default(),
        &|_| panic!("virtual selectors are removed during optimization"),
        &cell_value(virtual_cells, load_fixed),
        &cell_value(virtual_cells, load_advice),
        &cell_value(virtual_cells, load_instance),
        &|a| a,
        &|mut a, mut b| {
            a.append(&mut b);
            a
        },
        &|mut a, mut b| {
            a.append(&mut b);
            a
        },
        &|a, _| a,
    );
    cell_values.into_iter().collect()
}
