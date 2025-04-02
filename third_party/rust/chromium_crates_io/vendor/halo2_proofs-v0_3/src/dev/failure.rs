use std::collections::{BTreeMap, HashSet};
use std::fmt;

use group::ff::Field;

use super::{
    metadata,
    util::{self, AnyQuery},
    MockProver, Region,
};
use crate::{
    dev::Value,
    plonk::{Any, Column, ConstraintSystem, Expression, Gate, Instance},
};

mod emitter;

/// The location within the circuit at which a particular [`VerifyFailure`] occurred.
#[derive(Debug, PartialEq, Eq)]
pub enum FailureLocation {
    /// A location inside a region.
    InRegion {
        /// The region in which the failure occurred.
        region: metadata::Region,
        /// The offset (relative to the start of the region) at which the failure
        /// occurred.
        offset: usize,
    },
    /// A location outside of a region.
    OutsideRegion {
        /// The circuit row on which the failure occurred.
        row: usize,
    },
}

impl fmt::Display for FailureLocation {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InRegion { region, offset } => write!(f, "in {} at offset {}", region, offset),
            Self::OutsideRegion { row } => {
                write!(f, "outside any region, on row {}", row)
            }
        }
    }
}

impl FailureLocation {
    pub(super) fn find_expressions<'a, F: Field>(
        cs: &ConstraintSystem<F>,
        regions: &[Region],
        failure_row: usize,
        failure_expressions: impl Iterator<Item = &'a Expression<F>>,
    ) -> Self {
        let failure_columns: HashSet<Column<Any>> = failure_expressions
            .flat_map(|expression| {
                expression.evaluate(
                    &|_| vec![],
                    &|_| panic!("virtual selectors are removed during optimization"),
                    &|query| vec![cs.fixed_queries[query.index].0.into()],
                    &|query| vec![cs.advice_queries[query.index].0.into()],
                    &|query| vec![cs.instance_queries[query.index].0.into()],
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
                )
            })
            .collect();

        Self::find(regions, failure_row, failure_columns)
    }

    /// Figures out whether the given row and columns overlap an assigned region.
    pub(super) fn find(
        regions: &[Region],
        failure_row: usize,
        failure_columns: HashSet<Column<Any>>,
    ) -> Self {
        regions
            .iter()
            .enumerate()
            .find(|(_, r)| {
                if let Some((start, end)) = r.rows {
                    // We match the region if any input columns overlap, rather than all of
                    // them, because matching complex selector columns is hard. As long as
                    // regions are rectangles, and failures occur due to assignments entirely
                    // within single regions, "any" will be equivalent to "all". If these
                    // assumptions change, we'll start getting bug reports from users :)
                    (start..=end).contains(&failure_row) && !failure_columns.is_disjoint(&r.columns)
                } else {
                    // Zero-area region
                    false
                }
            })
            .map(|(r_i, r)| FailureLocation::InRegion {
                region: (r_i, r.name.clone()).into(),
                offset: failure_row - r.rows.unwrap().0,
            })
            .unwrap_or_else(|| FailureLocation::OutsideRegion { row: failure_row })
    }
}

/// The reasons why a particular circuit is not satisfied.
#[derive(Debug, PartialEq, Eq)]
pub enum VerifyFailure {
    /// A cell used in an active gate was not assigned to.
    CellNotAssigned {
        /// The index of the active gate.
        gate: metadata::Gate,
        /// The region in which this cell should be assigned.
        region: metadata::Region,
        /// The offset (relative to the start of the region) at which the active gate
        /// queries this cell.
        gate_offset: usize,
        /// The column in which this cell should be assigned.
        column: Column<Any>,
        /// The offset (relative to the start of the region) at which this cell should be
        /// assigned. This may be negative (for example, if a selector enables a gate at
        /// offset 0, but the gate uses `Rotation::prev()`).
        offset: isize,
    },
    /// An instance cell used in an active gate was not assigned to.
    InstanceCellNotAssigned {
        /// The index of the active gate.
        gate: metadata::Gate,
        /// The region in which this gate was activated.
        region: metadata::Region,
        /// The offset (relative to the start of the region) at which the active gate
        /// queries this cell.
        gate_offset: usize,
        /// The column in which this cell should be assigned.
        column: Column<Instance>,
        /// The absolute row at which this cell should be assigned.
        row: usize,
    },
    /// A constraint was not satisfied for a particular row.
    ConstraintNotSatisfied {
        /// The polynomial constraint that is not satisfied.
        constraint: metadata::Constraint,
        /// The location at which this constraint is not satisfied.
        ///
        /// `FailureLocation::OutsideRegion` is usually caused by a constraint that does
        /// not contain a selector, and as a result is active on every row.
        location: FailureLocation,
        /// The values of the virtual cells used by this constraint.
        cell_values: Vec<(metadata::VirtualCell, String)>,
    },
    /// A constraint was active on an unusable row, and is likely missing a selector.
    ConstraintPoisoned {
        /// The polynomial constraint that is not satisfied.
        constraint: metadata::Constraint,
    },
    /// A lookup input did not exist in its corresponding table.
    Lookup {
        /// The index of the lookup that is not satisfied. These indices are assigned in
        /// the order in which `ConstraintSystem::lookup` is called during
        /// `Circuit::configure`.
        lookup_index: usize,
        /// The location at which the lookup is not satisfied.
        ///
        /// `FailureLocation::InRegion` is most common, and may be due to the intentional
        /// use of a lookup (if its inputs are conditional on a complex selector), or an
        /// unintentional lookup constraint that overlaps the region (indicating that the
        /// lookup's inputs should be made conditional).
        ///
        /// `FailureLocation::OutsideRegion` is uncommon, and could mean that:
        /// - The input expressions do not correctly constrain a default value that exists
        ///   in the table when the lookup is not being used.
        /// - The input expressions use a column queried at a non-zero `Rotation`, and the
        ///   lookup is active on a row adjacent to an unrelated region.
        location: FailureLocation,
    },
    /// A permutation did not preserve the original value of a cell.
    Permutation {
        /// The column in which this permutation is not satisfied.
        column: metadata::Column,
        /// The location at which the permutation is not satisfied.
        location: FailureLocation,
    },
}

impl fmt::Display for VerifyFailure {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::CellNotAssigned {
                gate,
                region,
                gate_offset,
                column,
                offset,
            } => {
                write!(
                    f,
                    "{} uses {} at offset {}, which requires cell in column {:?} at offset {} to be assigned.",
                    region, gate, gate_offset, column, offset
                )
            }
            Self::InstanceCellNotAssigned {
                gate,
                region,
                gate_offset,
                column,
                row,
            } => {
                write!(
                    f,
                    "{} uses {} at offset {}, which requires cell in instance column {:?} at row {} to be assigned.",
                    region, gate, gate_offset, column, row
                )
            }
            Self::ConstraintNotSatisfied {
                constraint,
                location,
                cell_values,
            } => {
                writeln!(f, "{} is not satisfied {}", constraint, location)?;
                for (name, value) in cell_values {
                    writeln!(f, "- {} = {}", name, value)?;
                }
                Ok(())
            }
            Self::ConstraintPoisoned { constraint } => {
                write!(
                    f,
                    "{} is active on an unusable row - missing selector?",
                    constraint
                )
            }
            Self::Lookup {
                lookup_index,
                location,
            } => write!(f, "Lookup {} is not satisfied {}", lookup_index, location),
            Self::Permutation { column, location } => {
                write!(
                    f,
                    "Equality constraint not satisfied by cell ({:?}, {})",
                    column, location
                )
            }
        }
    }
}

/// Renders `VerifyFailure::CellNotAssigned`.
///
/// ```text
/// error: cell not assigned
///   Cell layout in region 'Faulty synthesis':
///     | Offset | A0 | A1 |
///     +--------+----+----+
///     |    0   | x0 |    |
///     |    1   |    |  X | <--{ X marks the spot! 🦜
///
///   Gate 'Equality check' (applied at offset 1) queries these cells.
/// ```
fn render_cell_not_assigned<F: Field>(
    gates: &[Gate<F>],
    gate: &metadata::Gate,
    region: &metadata::Region,
    gate_offset: usize,
    column: Column<Any>,
    offset: isize,
) {
    // Collect the necessary rendering information:
    // - The columns involved in this gate.
    // - How many cells are in each column.
    // - The grid of cell values, indexed by rotation.
    let mut columns = BTreeMap::<metadata::Column, usize>::default();
    let mut layout = BTreeMap::<i32, BTreeMap<metadata::Column, _>>::default();
    for (i, cell) in gates[gate.index].queried_cells().iter().enumerate() {
        let cell_column = cell.column.into();
        *columns.entry(cell_column).or_default() += 1;
        layout
            .entry(cell.rotation.0)
            .or_default()
            .entry(cell_column)
            .or_insert_with(|| {
                if cell.column == column && gate_offset as i32 + cell.rotation.0 == offset as i32 {
                    "X".to_string()
                } else {
                    format!("x{}", i)
                }
            });
    }

    eprintln!("error: cell not assigned");
    emitter::render_cell_layout(
        "  ",
        &FailureLocation::InRegion {
            region: region.clone(),
            offset: gate_offset,
        },
        &columns,
        &layout,
        |row_offset, rotation| {
            if (row_offset.unwrap() + rotation) as isize == offset {
                eprint!(" <--{{ X marks the spot! 🦜");
            }
        },
    );
    eprintln!();
    eprintln!(
        "  Gate '{}' (applied at offset {}) queries these cells.",
        gate.name, gate_offset
    );
}

/// Renders `VerifyFailure::ConstraintNotSatisfied`.
///
/// ```text
/// error: constraint not satisfied
///   Cell layout in region 'somewhere':
///     | Offset | A0 |
///     +--------+----+
///     |    0   | x0 | <--{ Gate 'foo' applied here
///     |    1   | x1 |
///
///   Constraint 'bar':
///     x1 + x1 * 0x100 + x1 * 0x10000 + x1 * 0x100_0000 - x0 = 0
///
///   Assigned cell values:
///     x0 = 0x5
///     x1 = 0x5
/// ```
fn render_constraint_not_satisfied<F: Field>(
    gates: &[Gate<F>],
    constraint: &metadata::Constraint,
    location: &FailureLocation,
    cell_values: &[(metadata::VirtualCell, String)],
) {
    // Collect the necessary rendering information:
    // - The columns involved in this constraint.
    // - How many cells are in each column.
    // - The grid of cell values, indexed by rotation.
    let mut columns = BTreeMap::<metadata::Column, usize>::default();
    let mut layout = BTreeMap::<i32, BTreeMap<metadata::Column, _>>::default();
    for (i, (cell, _)) in cell_values.iter().enumerate() {
        *columns.entry(cell.column).or_default() += 1;
        layout
            .entry(cell.rotation)
            .or_default()
            .entry(cell.column)
            .or_insert(format!("x{}", i));
    }

    eprintln!("error: constraint not satisfied");
    emitter::render_cell_layout("  ", location, &columns, &layout, |_, rotation| {
        if rotation == 0 {
            eprint!(" <--{{ Gate '{}' applied here", constraint.gate.name);
        }
    });

    // Print the unsatisfied constraint, in terms of the local variables.
    eprintln!();
    eprintln!("  Constraint '{}':", constraint.name);
    eprintln!(
        "    {} = 0",
        emitter::expression_to_string(
            &gates[constraint.gate.index].polynomials()[constraint.index],
            &layout
        )
    );

    // Print the map from local variables to assigned values.
    eprintln!();
    eprintln!("  Assigned cell values:");
    for (i, (_, value)) in cell_values.iter().enumerate() {
        eprintln!("    x{} = {}", i, value);
    }
}

/// Renders `VerifyFailure::Lookup`.
///
/// ```text
/// error: lookup input does not exist in table
///   (L0) ∉ (F0)
///
///   Lookup inputs:
///     L0 = x1 * x0 + (1 - x1) * 0x2
///     ^
///     | Cell layout in region 'Faulty synthesis':
///     |   | Offset | A0 | F1 |
///     |   +--------+----+----+
///     |   |    1   | x0 | x1 | <--{ Lookup inputs queried here
///     |
///     | Assigned cell values:
///     |   x0 = 0x5
///     |   x1 = 1
/// ```
fn render_lookup<F: Field>(
    prover: &MockProver<F>,
    lookup_index: usize,
    location: &FailureLocation,
) {
    let n = prover.n as i32;
    let cs = &prover.cs;
    let lookup = &cs.lookups[lookup_index];

    // Get the absolute row on which the lookup's inputs are being queried, so we can
    // fetch the input values.
    let row = match location {
        FailureLocation::InRegion { region, offset } => {
            prover.regions[region.index].rows.unwrap().0 + offset
        }
        FailureLocation::OutsideRegion { row } => *row,
    } as i32;

    // Recover the fixed columns from the table expressions. We don't allow composite
    // expressions for the table side of lookups.
    let table_columns = lookup.table_expressions.iter().map(|expr| {
        expr.evaluate(
            &|_| panic!("no constants in table expressions"),
            &|_| panic!("no selectors in table expressions"),
            &|query| format!("F{}", query.column_index),
            &|_| panic!("no advice columns in table expressions"),
            &|_| panic!("no instance columns in table expressions"),
            &|_| panic!("no negations in table expressions"),
            &|_, _| panic!("no sums in table expressions"),
            &|_, _| panic!("no products in table expressions"),
            &|_, _| panic!("no scaling in table expressions"),
        )
    });

    fn cell_value<'a, F: Field, Q: Into<AnyQuery> + Copy>(
        column_type: Any,
        load: impl Fn(Q) -> Value<F> + 'a,
    ) -> impl Fn(Q) -> BTreeMap<metadata::VirtualCell, String> + 'a {
        move |query| {
            let AnyQuery {
                column_index,
                rotation,
                ..
            } = query.into();
            Some((
                ((column_type, column_index).into(), rotation.0).into(),
                match load(query) {
                    Value::Real(v) => util::format_value(v),
                    Value::Poison => unreachable!(),
                },
            ))
            .into_iter()
            .collect()
        }
    }

    eprintln!("error: lookup input does not exist in table");
    eprint!("  (");
    for i in 0..lookup.input_expressions.len() {
        eprint!("{}L{}", if i == 0 { "" } else { ", " }, i);
    }
    eprint!(") ∉ (");
    for (i, column) in table_columns.enumerate() {
        eprint!("{}{}", if i == 0 { "" } else { ", " }, column);
    }
    eprintln!(")");

    eprintln!();
    eprintln!("  Lookup inputs:");
    for (i, input) in lookup.input_expressions.iter().enumerate() {
        // Fetch the cell values (since we don't store them in VerifyFailure::Lookup).
        let cell_values = input.evaluate(
            &|_| BTreeMap::default(),
            &|_| panic!("virtual selectors are removed during optimization"),
            &cell_value(
                Any::Fixed,
                &util::load(n, row, &cs.fixed_queries, &prover.fixed),
            ),
            &cell_value(
                Any::Advice,
                &util::load(n, row, &cs.advice_queries, &prover.advice),
            ),
            &cell_value(
                Any::Instance,
                &util::load_instance(n, row, &cs.instance_queries, &prover.instance),
            ),
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

        // Collect the necessary rendering information:
        // - The columns involved in this constraint.
        // - How many cells are in each column.
        // - The grid of cell values, indexed by rotation.
        let mut columns = BTreeMap::<metadata::Column, usize>::default();
        let mut layout = BTreeMap::<i32, BTreeMap<metadata::Column, _>>::default();
        for (i, (cell, _)) in cell_values.iter().enumerate() {
            *columns.entry(cell.column).or_default() += 1;
            layout
                .entry(cell.rotation)
                .or_default()
                .entry(cell.column)
                .or_insert(format!("x{}", i));
        }

        if i != 0 {
            eprintln!();
        }
        eprintln!(
            "    L{} = {}",
            i,
            emitter::expression_to_string(input, &layout)
        );
        eprintln!("    ^");
        emitter::render_cell_layout("    | ", location, &columns, &layout, |_, rotation| {
            if rotation == 0 {
                eprint!(" <--{{ Lookup inputs queried here");
            }
        });

        // Print the map from local variables to assigned values.
        eprintln!("    |");
        eprintln!("    | Assigned cell values:");
        for (i, (_, value)) in cell_values.iter().enumerate() {
            eprintln!("    |   x{} = {}", i, value);
        }
    }
}

impl VerifyFailure {
    /// Emits this failure in pretty-printed format to stderr.
    pub(super) fn emit<F: Field>(&self, prover: &MockProver<F>) {
        match self {
            Self::CellNotAssigned {
                gate,
                region,
                gate_offset,
                column,
                offset,
            } => render_cell_not_assigned(
                &prover.cs.gates,
                gate,
                region,
                *gate_offset,
                *column,
                *offset,
            ),
            Self::ConstraintNotSatisfied {
                constraint,
                location,
                cell_values,
            } => {
                render_constraint_not_satisfied(&prover.cs.gates, constraint, location, cell_values)
            }
            Self::Lookup {
                lookup_index,
                location,
            } => render_lookup(prover, *lookup_index, location),
            _ => eprintln!("{}", self),
        }
    }
}
