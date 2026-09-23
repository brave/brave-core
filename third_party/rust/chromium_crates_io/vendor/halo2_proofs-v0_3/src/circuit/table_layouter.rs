//! Implementations of common table layouters.

use std::{
    collections::HashMap,
    fmt::{self, Debug},
};

use ff::Field;

use crate::plonk::{Assigned, Assignment, Error, TableColumn, TableError};

use super::Value;

/// Helper trait for implementing a custom [`Layouter`].
///
/// This trait is used for implementing table assignments.
///
/// [`Layouter`]: super::Layouter
pub trait TableLayouter<F: Field>: std::fmt::Debug {
    /// Assigns a fixed value to a table cell.
    ///
    /// Returns an error if the table cell has already been assigned to.
    fn assign_cell<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: TableColumn,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<(), Error>;
}

/// The default value to fill a table column with.
///
/// - The outer `Option` tracks whether the value in row 0 of the table column has been
///   assigned yet. This will always be `Some` once a valid table has been completely
///   assigned.
/// - The inner `Value` tracks whether the underlying `Assignment` is evaluating
///   witnesses or not.
type DefaultTableValue<F> = Option<Value<Assigned<F>>>;

pub(crate) struct SimpleTableLayouter<'r, 'a, F: Field, CS: Assignment<F> + 'a> {
    cs: &'a mut CS,
    used_columns: &'r [TableColumn],
    // maps from a fixed column to a pair (default value, vector saying which rows are assigned)
    pub(crate) default_and_assigned: HashMap<TableColumn, (DefaultTableValue<F>, Vec<bool>)>,
}

impl<'r, 'a, F: Field, CS: Assignment<F> + 'a> fmt::Debug for SimpleTableLayouter<'r, 'a, F, CS> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("SimpleTableLayouter")
            .field("used_columns", &self.used_columns)
            .field("default_and_assigned", &self.default_and_assigned)
            .finish()
    }
}

impl<'r, 'a, F: Field, CS: Assignment<F> + 'a> SimpleTableLayouter<'r, 'a, F, CS> {
    pub(crate) fn new(cs: &'a mut CS, used_columns: &'r [TableColumn]) -> Self {
        SimpleTableLayouter {
            cs,
            used_columns,
            default_and_assigned: HashMap::default(),
        }
    }
}

impl<'r, 'a, F: Field, CS: Assignment<F> + 'a> TableLayouter<F>
    for SimpleTableLayouter<'r, 'a, F, CS>
{
    fn assign_cell<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: TableColumn,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<(), Error> {
        if self.used_columns.contains(&column) {
            return Err(Error::TableError(TableError::UsedColumn(column)));
        }

        let entry = self.default_and_assigned.entry(column).or_default();

        let mut value = Value::unknown();
        self.cs.assign_fixed(
            annotation,
            column.inner(),
            offset, // tables are always assigned starting at row 0
            || {
                let res = to();
                value = res;
                res
            },
        )?;

        match (entry.0.is_none(), offset) {
            // Use the value at offset 0 as the default value for this table column.
            (true, 0) => entry.0 = Some(value),
            // Since there is already an existing default value for this table column,
            // the caller should not be attempting to assign another value at offset 0.
            (false, 0) => {
                return Err(Error::TableError(TableError::OverwriteDefault(
                    column,
                    format!("{:?}", entry.0.unwrap()),
                    format!("{:?}", value),
                )))
            }
            _ => (),
        }
        if entry.1.len() <= offset {
            entry.1.resize(offset + 1, false);
        }
        entry.1[offset] = true;

        Ok(())
    }
}

pub(crate) fn compute_table_lengths<F: Debug>(
    default_and_assigned: &HashMap<TableColumn, (DefaultTableValue<F>, Vec<bool>)>,
) -> Result<usize, Error> {
    let column_lengths: Result<Vec<_>, Error> = default_and_assigned
        .iter()
        .map(|(col, (default_value, assigned))| {
            if default_value.is_none() || assigned.is_empty() {
                return Err(Error::TableError(TableError::ColumnNotAssigned(*col)));
            }
            if assigned.iter().all(|b| *b) {
                // All values in the column have been assigned
                Ok((col, assigned.len()))
            } else {
                Err(Error::TableError(TableError::ColumnNotAssigned(*col)))
            }
        })
        .collect();
    let column_lengths = column_lengths?;
    column_lengths
        .into_iter()
        .try_fold((None, 0), |acc, (col, col_len)| {
            if acc.1 == 0 || acc.1 == col_len {
                Ok((Some(*col), col_len))
            } else {
                let mut cols = [(*col, col_len), (acc.0.unwrap(), acc.1)];
                cols.sort();
                Err(Error::TableError(TableError::UnevenColumnLengths(
                    cols[0], cols[1],
                )))
            }
        })
        .map(|col_len| col_len.1)
}

#[cfg(test)]
mod tests {
    use pasta_curves::Fp;

    use crate::{
        circuit::{Layouter, SimpleFloorPlanner},
        dev::MockProver,
        plonk::{Circuit, ConstraintSystem},
        poly::Rotation,
    };

    use super::*;

    #[test]
    fn table_no_default() {
        const K: u32 = 4;

        #[derive(Clone)]
        struct FaultyCircuitConfig {
            table: TableColumn,
        }

        struct FaultyCircuit;

        impl Circuit<Fp> for FaultyCircuit {
            type Config = FaultyCircuitConfig;

            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self
            }

            fn configure(meta: &mut ConstraintSystem<Fp>) -> Self::Config {
                let a = meta.advice_column();
                let table = meta.lookup_table_column();

                meta.lookup(|cells| {
                    let a = cells.query_advice(a, Rotation::cur());
                    vec![(a, table)]
                });

                Self::Config { table }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<Fp>,
            ) -> Result<(), Error> {
                layouter.assign_table(
                    || "duplicate assignment",
                    |mut table| {
                        table.assign_cell(
                            || "default",
                            config.table,
                            1,
                            || Value::known(Fp::zero()),
                        )
                    },
                )
            }
        }

        let prover = MockProver::run(K, &FaultyCircuit, vec![]);
        assert_eq!(
            format!("{}", prover.unwrap_err()),
            "TableColumn { inner: Column { index: 0, column_type: Fixed } } not fully assigned. Help: assign a value at offset 0."
        );
    }

    #[test]
    fn table_overwrite_default() {
        const K: u32 = 4;

        #[derive(Clone)]
        struct FaultyCircuitConfig {
            table: TableColumn,
        }

        struct FaultyCircuit;

        impl Circuit<Fp> for FaultyCircuit {
            type Config = FaultyCircuitConfig;

            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self
            }

            fn configure(meta: &mut ConstraintSystem<Fp>) -> Self::Config {
                let a = meta.advice_column();
                let table = meta.lookup_table_column();

                meta.lookup(|cells| {
                    let a = cells.query_advice(a, Rotation::cur());
                    vec![(a, table)]
                });

                Self::Config { table }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<Fp>,
            ) -> Result<(), Error> {
                layouter.assign_table(
                    || "duplicate assignment",
                    |mut table| {
                        table.assign_cell(
                            || "default",
                            config.table,
                            0,
                            || Value::known(Fp::zero()),
                        )?;
                        table.assign_cell(
                            || "duplicate",
                            config.table,
                            0,
                            || Value::known(Fp::zero()),
                        )
                    },
                )
            }
        }

        let prover = MockProver::run(K, &FaultyCircuit, vec![]);
        assert_eq!(
            format!("{}", prover.unwrap_err()),
            "Attempted to overwrite default value Value { inner: Some(Trivial(0x0000000000000000000000000000000000000000000000000000000000000000)) } with Value { inner: Some(Trivial(0x0000000000000000000000000000000000000000000000000000000000000000)) } in TableColumn { inner: Column { index: 0, column_type: Fixed } }"
        );
    }

    #[test]
    fn table_reuse_column() {
        const K: u32 = 4;

        #[derive(Clone)]
        struct FaultyCircuitConfig {
            table: TableColumn,
        }

        struct FaultyCircuit;

        impl Circuit<Fp> for FaultyCircuit {
            type Config = FaultyCircuitConfig;

            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self
            }

            fn configure(meta: &mut ConstraintSystem<Fp>) -> Self::Config {
                let a = meta.advice_column();
                let table = meta.lookup_table_column();

                meta.lookup(|cells| {
                    let a = cells.query_advice(a, Rotation::cur());
                    vec![(a, table)]
                });

                Self::Config { table }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<Fp>,
            ) -> Result<(), Error> {
                layouter.assign_table(
                    || "first assignment",
                    |mut table| {
                        table.assign_cell(
                            || "default",
                            config.table,
                            0,
                            || Value::known(Fp::zero()),
                        )
                    },
                )?;

                layouter.assign_table(
                    || "reuse",
                    |mut table| {
                        table.assign_cell(|| "reuse", config.table, 1, || Value::known(Fp::zero()))
                    },
                )
            }
        }

        let prover = MockProver::run(K, &FaultyCircuit, vec![]);
        assert_eq!(
            format!("{}", prover.unwrap_err()),
            "TableColumn { inner: Column { index: 0, column_type: Fixed } } has already been used"
        );
    }

    #[test]
    fn table_uneven_columns() {
        const K: u32 = 4;

        #[derive(Clone)]
        struct FaultyCircuitConfig {
            table: (TableColumn, TableColumn),
        }

        struct FaultyCircuit;

        impl Circuit<Fp> for FaultyCircuit {
            type Config = FaultyCircuitConfig;

            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self
            }

            fn configure(meta: &mut ConstraintSystem<Fp>) -> Self::Config {
                let a = meta.advice_column();
                let table = (meta.lookup_table_column(), meta.lookup_table_column());
                meta.lookup(|cells| {
                    let a = cells.query_advice(a, Rotation::cur());

                    vec![(a.clone(), table.0), (a, table.1)]
                });

                Self::Config { table }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<Fp>,
            ) -> Result<(), Error> {
                layouter.assign_table(
                    || "table with uneven columns",
                    |mut table| {
                        table.assign_cell(|| "", config.table.0, 0, || Value::known(Fp::zero()))?;
                        table.assign_cell(|| "", config.table.0, 1, || Value::known(Fp::zero()))?;

                        table.assign_cell(|| "", config.table.1, 0, || Value::known(Fp::zero()))
                    },
                )
            }
        }

        let prover = MockProver::run(K, &FaultyCircuit, vec![]);
        assert_eq!(
            format!("{}", prover.unwrap_err()),
            "TableColumn { inner: Column { index: 0, column_type: Fixed } } has length 2 while TableColumn { inner: Column { index: 1, column_type: Fixed } } has length 1"
        );
    }
}
