//! Implementations of common circuit layouters.

use std::cmp;
use std::collections::HashSet;
use std::fmt;

use ff::Field;

use super::{Cell, RegionIndex, Value};
use crate::plonk::{Advice, Any, Assigned, Column, Error, Fixed, Instance, Selector, TableColumn};

/// Helper trait for implementing a custom [`Layouter`].
///
/// This trait is used for implementing region assignments:
///
/// ```ignore
/// impl<'a, F: FieldExt, C: Chip<F>, CS: Assignment<F> + 'a> Layouter<C> for MyLayouter<'a, C, CS> {
///     fn assign_region(
///         &mut self,
///         assignment: impl FnOnce(Region<'_, F, C>) -> Result<(), Error>,
///     ) -> Result<(), Error> {
///         let region_index = self.regions.len();
///         self.regions.push(self.current_gate);
///
///         let mut region = MyRegion::new(self, region_index);
///         {
///             let region: &mut dyn RegionLayouter<F> = &mut region;
///             assignment(region.into())?;
///         }
///         self.current_gate += region.row_count;
///
///         Ok(())
///     }
/// }
/// ```
///
/// TODO: It would be great if we could constrain the columns in these types to be
/// "logical" columns that are guaranteed to correspond to the chip (and have come from
/// `Chip::Config`).
///
/// [`Layouter`]: super::Layouter
pub trait RegionLayouter<F: Field>: fmt::Debug {
    /// Enables a selector at the given offset.
    fn enable_selector<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        selector: &Selector,
        offset: usize,
    ) -> Result<(), Error>;

    /// Assign an advice column value (witness)
    fn assign_advice<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error>;

    /// Assigns a constant value to the column `advice` at `offset` within this region.
    ///
    /// The constant value will be assigned to a cell within one of the fixed columns
    /// configured via `ConstraintSystem::enable_constant`.
    ///
    /// Returns the advice cell that has been equality-constrained to the constant.
    fn assign_advice_from_constant<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        constant: Assigned<F>,
    ) -> Result<Cell, Error>;

    /// Assign the value of the instance column's cell at absolute location
    /// `row` to the column `advice` at `offset` within this region.
    ///
    /// Returns the advice cell that has been equality-constrained to the
    /// instance cell, and its value if known.
    fn assign_advice_from_instance<'v>(
        &mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        instance: Column<Instance>,
        row: usize,
        advice: Column<Advice>,
        offset: usize,
    ) -> Result<(Cell, Value<F>), Error>;

    /// Returns the value of the instance column's cell at absolute location `row`.
    fn instance_value(&mut self, instance: Column<Instance>, row: usize)
        -> Result<Value<F>, Error>;

    /// Assigns a fixed value
    fn assign_fixed<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Fixed>,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error>;

    /// Constrains a cell to have a constant value.
    ///
    /// Returns an error if the cell is in a column where equality has not been enabled.
    fn constrain_constant(&mut self, cell: Cell, constant: Assigned<F>) -> Result<(), Error>;

    /// Constraint two cells to have the same value.
    ///
    /// Returns an error if either of the cells is not within the given permutation.
    fn constrain_equal(&mut self, left: Cell, right: Cell) -> Result<(), Error>;
}

/// Helper trait for implementing a custom [`Layouter`].
///
/// This trait is used for implementing table assignments.
///
/// [`Layouter`]: super::Layouter
pub trait TableLayouter<F: Field>: fmt::Debug {
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

/// The shape of a region. For a region at a certain index, we track
/// the set of columns it uses as well as the number of rows it uses.
#[derive(Clone, Debug)]
pub struct RegionShape {
    pub(super) region_index: RegionIndex,
    pub(super) columns: HashSet<RegionColumn>,
    pub(super) row_count: usize,
}

/// The virtual column involved in a region. This includes concrete columns,
/// as well as selectors that are not concrete columns at this stage.
#[derive(Eq, PartialEq, Copy, Clone, Debug, Hash)]
pub enum RegionColumn {
    /// Concrete column
    Column(Column<Any>),
    /// Virtual column representing a (boolean) selector
    Selector(Selector),
}

impl From<Column<Any>> for RegionColumn {
    fn from(column: Column<Any>) -> RegionColumn {
        RegionColumn::Column(column)
    }
}

impl From<Selector> for RegionColumn {
    fn from(selector: Selector) -> RegionColumn {
        RegionColumn::Selector(selector)
    }
}

impl Ord for RegionColumn {
    fn cmp(&self, other: &Self) -> cmp::Ordering {
        match (self, other) {
            (Self::Column(ref a), Self::Column(ref b)) => a.cmp(b),
            (Self::Selector(ref a), Self::Selector(ref b)) => a.0.cmp(&b.0),
            (Self::Column(_), Self::Selector(_)) => cmp::Ordering::Less,
            (Self::Selector(_), Self::Column(_)) => cmp::Ordering::Greater,
        }
    }
}

impl PartialOrd for RegionColumn {
    fn partial_cmp(&self, other: &Self) -> Option<cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl RegionShape {
    /// Create a new `RegionShape` for a region at `region_index`.
    pub fn new(region_index: RegionIndex) -> Self {
        RegionShape {
            region_index,
            columns: HashSet::default(),
            row_count: 0,
        }
    }

    /// Get the `region_index` of a `RegionShape`.
    pub fn region_index(&self) -> RegionIndex {
        self.region_index
    }

    /// Get a reference to the set of `columns` used in a `RegionShape`.
    pub fn columns(&self) -> &HashSet<RegionColumn> {
        &self.columns
    }

    /// Get the `row_count` of a `RegionShape`.
    pub fn row_count(&self) -> usize {
        self.row_count
    }
}

impl<F: Field> RegionLayouter<F> for RegionShape {
    fn enable_selector<'v>(
        &'v mut self,
        _: &'v (dyn Fn() -> String + 'v),
        selector: &Selector,
        offset: usize,
    ) -> Result<(), Error> {
        // Track the selector's fixed column as part of the region's shape.
        self.columns.insert((*selector).into());
        self.row_count = cmp::max(self.row_count, offset + 1);
        Ok(())
    }

    fn assign_advice<'v>(
        &'v mut self,
        _: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        _to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error> {
        self.columns.insert(Column::<Any>::from(column).into());
        self.row_count = cmp::max(self.row_count, offset + 1);

        Ok(Cell {
            region_index: self.region_index,
            row_offset: offset,
            column: column.into(),
        })
    }

    fn assign_advice_from_constant<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        constant: Assigned<F>,
    ) -> Result<Cell, Error> {
        // The rest is identical to witnessing an advice cell.
        self.assign_advice(annotation, column, offset, &mut || Value::known(constant))
    }

    fn assign_advice_from_instance<'v>(
        &mut self,
        _: &'v (dyn Fn() -> String + 'v),
        _: Column<Instance>,
        _: usize,
        advice: Column<Advice>,
        offset: usize,
    ) -> Result<(Cell, Value<F>), Error> {
        self.columns.insert(Column::<Any>::from(advice).into());
        self.row_count = cmp::max(self.row_count, offset + 1);

        Ok((
            Cell {
                region_index: self.region_index,
                row_offset: offset,
                column: advice.into(),
            },
            Value::unknown(),
        ))
    }

    fn instance_value(
        &mut self,
        _instance: Column<Instance>,
        _row: usize,
    ) -> Result<Value<F>, Error> {
        Ok(Value::unknown())
    }

    fn assign_fixed<'v>(
        &'v mut self,
        _: &'v (dyn Fn() -> String + 'v),
        column: Column<Fixed>,
        offset: usize,
        _to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error> {
        self.columns.insert(Column::<Any>::from(column).into());
        self.row_count = cmp::max(self.row_count, offset + 1);

        Ok(Cell {
            region_index: self.region_index,
            row_offset: offset,
            column: column.into(),
        })
    }

    fn constrain_constant(&mut self, _cell: Cell, _constant: Assigned<F>) -> Result<(), Error> {
        // Global constants don't affect the region shape.
        Ok(())
    }

    fn constrain_equal(&mut self, _left: Cell, _right: Cell) -> Result<(), Error> {
        // Equality constraints don't affect the region shape.
        Ok(())
    }
}
