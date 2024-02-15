use core::cmp::max;
use core::ops::{Add, Mul};
use ff::Field;
use std::{
    convert::TryFrom,
    ops::{Neg, Sub},
};

use super::{lookup, permutation, Assigned, Error};
use crate::{
    circuit::{Layouter, Region, Value},
    poly::Rotation,
};

mod compress_selectors;

/// A column type
pub trait ColumnType:
    'static + Sized + Copy + std::fmt::Debug + PartialEq + Eq + Into<Any>
{
}

/// A column with an index and type
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct Column<C: ColumnType> {
    index: usize,
    column_type: C,
}

impl<C: ColumnType> Column<C> {
    #[cfg(test)]
    pub(crate) fn new(index: usize, column_type: C) -> Self {
        Column { index, column_type }
    }

    pub(crate) fn index(&self) -> usize {
        self.index
    }

    /// Type of this column.
    pub fn column_type(&self) -> &C {
        &self.column_type
    }
}

impl<C: ColumnType> Ord for Column<C> {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        // This ordering is consensus-critical! The layouters rely on deterministic column
        // orderings.
        match self.column_type.into().cmp(&other.column_type.into()) {
            // Indices are assigned within column types.
            std::cmp::Ordering::Equal => self.index.cmp(&other.index),
            order => order,
        }
    }
}

impl<C: ColumnType> PartialOrd for Column<C> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

/// An advice column
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct Advice;

/// A fixed column
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct Fixed;

/// An instance column
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct Instance;

/// An enum over the Advice, Fixed, Instance structs
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub enum Any {
    /// An Advice variant
    Advice,
    /// A Fixed variant
    Fixed,
    /// An Instance variant
    Instance,
}

impl Ord for Any {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        // This ordering is consensus-critical! The layouters rely on deterministic column
        // orderings.
        match (self, other) {
            (Any::Instance, Any::Instance)
            | (Any::Advice, Any::Advice)
            | (Any::Fixed, Any::Fixed) => std::cmp::Ordering::Equal,
            // Across column types, sort Instance < Advice < Fixed.
            (Any::Instance, Any::Advice)
            | (Any::Advice, Any::Fixed)
            | (Any::Instance, Any::Fixed) => std::cmp::Ordering::Less,
            (Any::Fixed, Any::Instance)
            | (Any::Fixed, Any::Advice)
            | (Any::Advice, Any::Instance) => std::cmp::Ordering::Greater,
        }
    }
}

impl PartialOrd for Any {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl ColumnType for Advice {}
impl ColumnType for Fixed {}
impl ColumnType for Instance {}
impl ColumnType for Any {}

impl From<Advice> for Any {
    fn from(_: Advice) -> Any {
        Any::Advice
    }
}

impl From<Fixed> for Any {
    fn from(_: Fixed) -> Any {
        Any::Fixed
    }
}

impl From<Instance> for Any {
    fn from(_: Instance) -> Any {
        Any::Instance
    }
}

impl From<Column<Advice>> for Column<Any> {
    fn from(advice: Column<Advice>) -> Column<Any> {
        Column {
            index: advice.index(),
            column_type: Any::Advice,
        }
    }
}

impl From<Column<Fixed>> for Column<Any> {
    fn from(advice: Column<Fixed>) -> Column<Any> {
        Column {
            index: advice.index(),
            column_type: Any::Fixed,
        }
    }
}

impl From<Column<Instance>> for Column<Any> {
    fn from(advice: Column<Instance>) -> Column<Any> {
        Column {
            index: advice.index(),
            column_type: Any::Instance,
        }
    }
}

impl TryFrom<Column<Any>> for Column<Advice> {
    type Error = &'static str;

    fn try_from(any: Column<Any>) -> Result<Self, Self::Error> {
        match any.column_type() {
            Any::Advice => Ok(Column {
                index: any.index(),
                column_type: Advice,
            }),
            _ => Err("Cannot convert into Column<Advice>"),
        }
    }
}

impl TryFrom<Column<Any>> for Column<Fixed> {
    type Error = &'static str;

    fn try_from(any: Column<Any>) -> Result<Self, Self::Error> {
        match any.column_type() {
            Any::Fixed => Ok(Column {
                index: any.index(),
                column_type: Fixed,
            }),
            _ => Err("Cannot convert into Column<Fixed>"),
        }
    }
}

impl TryFrom<Column<Any>> for Column<Instance> {
    type Error = &'static str;

    fn try_from(any: Column<Any>) -> Result<Self, Self::Error> {
        match any.column_type() {
            Any::Instance => Ok(Column {
                index: any.index(),
                column_type: Instance,
            }),
            _ => Err("Cannot convert into Column<Instance>"),
        }
    }
}

/// A selector, representing a fixed boolean value per row of the circuit.
///
/// Selectors can be used to conditionally enable (portions of) gates:
/// ```
/// use halo2_proofs::poly::Rotation;
/// # use halo2_proofs::pasta::Fp;
/// # use halo2_proofs::plonk::ConstraintSystem;
///
/// # let mut meta = ConstraintSystem::<Fp>::default();
/// let a = meta.advice_column();
/// let b = meta.advice_column();
/// let s = meta.selector();
///
/// meta.create_gate("foo", |meta| {
///     let a = meta.query_advice(a, Rotation::prev());
///     let b = meta.query_advice(b, Rotation::cur());
///     let s = meta.query_selector(s);
///
///     // On rows where the selector is enabled, a is constrained to equal b.
///     // On rows where the selector is disabled, a and b can take any value.
///     vec![s * (a - b)]
/// });
/// ```
///
/// Selectors are disabled on all rows by default, and must be explicitly enabled on each
/// row when required:
/// ```
/// use group::ff::Field;
/// use halo2_proofs::{
///     circuit::{Chip, Layouter, Value},
///     plonk::{Advice, Column, Error, Selector},
/// };
/// # use halo2_proofs::plonk::Fixed;
///
/// struct Config {
///     a: Column<Advice>,
///     b: Column<Advice>,
///     s: Selector,
/// }
///
/// fn circuit_logic<F: Field, C: Chip<F>>(chip: C, mut layouter: impl Layouter<F>) -> Result<(), Error> {
///     let config = chip.config();
///     # let config: Config = todo!();
///     layouter.assign_region(|| "bar", |mut region| {
///         region.assign_advice(|| "a", config.a, 0, || Value::known(F::ONE))?;
///         region.assign_advice(|| "a", config.b, 1, || Value::known(F::ONE))?;
///         config.s.enable(&mut region, 1)
///     })?;
///     Ok(())
/// }
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Selector(pub(crate) usize, bool);

impl Selector {
    /// Enable this selector at the given offset within the given region.
    pub fn enable<F: Field>(&self, region: &mut Region<F>, offset: usize) -> Result<(), Error> {
        region.enable_selector(|| "", self, offset)
    }

    /// Is this selector "simple"? Simple selectors can only be multiplied
    /// by expressions that contain no other simple selectors.
    pub fn is_simple(&self) -> bool {
        self.1
    }
}

/// Query of fixed column at a certain relative location
#[derive(Copy, Clone, Debug)]
pub struct FixedQuery {
    /// Query index
    pub(crate) index: usize,
    /// Column index
    pub(crate) column_index: usize,
    /// Rotation of this query
    pub(crate) rotation: Rotation,
}

/// Query of advice column at a certain relative location
#[derive(Copy, Clone, Debug)]
pub struct AdviceQuery {
    /// Query index
    pub(crate) index: usize,
    /// Column index
    pub(crate) column_index: usize,
    /// Rotation of this query
    pub(crate) rotation: Rotation,
}

/// Query of instance column at a certain relative location
#[derive(Copy, Clone, Debug)]
pub struct InstanceQuery {
    /// Query index
    pub(crate) index: usize,
    /// Column index
    pub(crate) column_index: usize,
    /// Rotation of this query
    pub(crate) rotation: Rotation,
}

/// A fixed column of a lookup table.
///
/// A lookup table can be loaded into this column via [`Layouter::assign_table`]. Columns
/// can currently only contain a single table, but they may be used in multiple lookup
/// arguments via [`ConstraintSystem::lookup`].
///
/// Lookup table columns are always "encumbered" by the lookup arguments they are used in;
/// they cannot simultaneously be used as general fixed columns.
///
/// [`Layouter::assign_table`]: crate::circuit::Layouter::assign_table
#[derive(Clone, Copy, Debug, Eq, PartialEq, Hash)]
pub struct TableColumn {
    /// The fixed column that this table column is stored in.
    ///
    /// # Security
    ///
    /// This inner column MUST NOT be exposed in the public API, or else chip developers
    /// can load lookup tables into their circuits without default-value-filling the
    /// columns, which can cause soundness bugs.
    inner: Column<Fixed>,
}

impl TableColumn {
    pub(crate) fn inner(&self) -> Column<Fixed> {
        self.inner
    }

    /// Enable equality on this TableColumn.
    pub fn enable_equality<F: Field>(&self, meta: &mut ConstraintSystem<F>) {
        meta.enable_equality(self.inner)
    }
}

/// This trait allows a [`Circuit`] to direct some backend to assign a witness
/// for a constraint system.
pub trait Assignment<F: Field> {
    /// Creates a new region and enters into it.
    ///
    /// Panics if we are currently in a region (if `exit_region` was not called).
    ///
    /// Not intended for downstream consumption; use [`Layouter::assign_region`] instead.
    ///
    /// [`Layouter::assign_region`]: crate::circuit::Layouter#method.assign_region
    fn enter_region<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR;

    /// Exits the current region.
    ///
    /// Panics if we are not currently in a region (if `enter_region` was not called).
    ///
    /// Not intended for downstream consumption; use [`Layouter::assign_region`] instead.
    ///
    /// [`Layouter::assign_region`]: crate::circuit::Layouter#method.assign_region
    fn exit_region(&mut self);

    /// Enables a selector at the given row.
    fn enable_selector<A, AR>(
        &mut self,
        annotation: A,
        selector: &Selector,
        row: usize,
    ) -> Result<(), Error>
    where
        A: FnOnce() -> AR,
        AR: Into<String>;

    /// Queries the cell of an instance column at a particular absolute row.
    ///
    /// Returns the cell's value, if known.
    fn query_instance(&self, column: Column<Instance>, row: usize) -> Result<Value<F>, Error>;

    /// Assign an advice column value (witness)
    fn assign_advice<V, VR, A, AR>(
        &mut self,
        annotation: A,
        column: Column<Advice>,
        row: usize,
        to: V,
    ) -> Result<(), Error>
    where
        V: FnOnce() -> Value<VR>,
        VR: Into<Assigned<F>>,
        A: FnOnce() -> AR,
        AR: Into<String>;

    /// Assign a fixed value
    fn assign_fixed<V, VR, A, AR>(
        &mut self,
        annotation: A,
        column: Column<Fixed>,
        row: usize,
        to: V,
    ) -> Result<(), Error>
    where
        V: FnOnce() -> Value<VR>,
        VR: Into<Assigned<F>>,
        A: FnOnce() -> AR,
        AR: Into<String>;

    /// Assign two cells to have the same value
    fn copy(
        &mut self,
        left_column: Column<Any>,
        left_row: usize,
        right_column: Column<Any>,
        right_row: usize,
    ) -> Result<(), Error>;

    /// Fills a fixed `column` starting from the given `row` with value `to`.
    fn fill_from_row(
        &mut self,
        column: Column<Fixed>,
        row: usize,
        to: Value<Assigned<F>>,
    ) -> Result<(), Error>;

    /// Creates a new (sub)namespace and enters into it.
    ///
    /// Not intended for downstream consumption; use [`Layouter::namespace`] instead.
    ///
    /// [`Layouter::namespace`]: crate::circuit::Layouter#method.namespace
    fn push_namespace<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR;

    /// Exits out of the existing namespace.
    ///
    /// Not intended for downstream consumption; use [`Layouter::namespace`] instead.
    ///
    /// [`Layouter::namespace`]: crate::circuit::Layouter#method.namespace
    fn pop_namespace(&mut self, gadget_name: Option<String>);
}

/// A floor planning strategy for a circuit.
///
/// The floor planner is chip-agnostic and applies its strategy to the circuit it is used
/// within.
pub trait FloorPlanner {
    /// Given the provided `cs`, synthesize the given circuit.
    ///
    /// `constants` is the list of fixed columns that the layouter may use to assign
    /// global constant values. These columns will all have been equality-enabled.
    ///
    /// Internally, a floor planner will perform the following operations:
    /// - Instantiate a [`Layouter`] for this floor planner.
    /// - Perform any necessary setup or measurement tasks, which may involve one or more
    ///   calls to `Circuit::default().synthesize(config, &mut layouter)`.
    /// - Call `circuit.synthesize(config, &mut layouter)` exactly once.
    fn synthesize<F: Field, CS: Assignment<F>, C: Circuit<F>>(
        cs: &mut CS,
        circuit: &C,
        config: C::Config,
        constants: Vec<Column<Fixed>>,
    ) -> Result<(), Error>;
}

/// This is a trait that circuits provide implementations for so that the
/// backend prover can ask the circuit to synthesize using some given
/// [`ConstraintSystem`] implementation.
pub trait Circuit<F: Field> {
    /// This is a configuration object that stores things like columns.
    type Config: Clone;
    /// The floor planner used for this circuit. This is an associated type of the
    /// `Circuit` trait because its behaviour is circuit-critical.
    type FloorPlanner: FloorPlanner;

    /// Returns a copy of this circuit with no witness values (i.e. all witnesses set to
    /// `None`). For most circuits, this will be equal to `Self::default()`.
    fn without_witnesses(&self) -> Self;

    /// The circuit is given an opportunity to describe the exact gate
    /// arrangement, column arrangement, etc.
    fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config;

    /// Given the provided `cs`, synthesize the circuit. The concrete type of
    /// the caller will be different depending on the context, and they may or
    /// may not expect to have a witness present.
    fn synthesize(&self, config: Self::Config, layouter: impl Layouter<F>) -> Result<(), Error>;
}

/// Low-degree expression representing an identity that must hold over the committed columns.
#[derive(Clone)]
pub enum Expression<F> {
    /// This is a constant polynomial
    Constant(F),
    /// This is a virtual selector
    Selector(Selector),
    /// This is a fixed column queried at a certain relative location
    Fixed(FixedQuery),
    /// This is an advice (witness) column queried at a certain relative location
    Advice(AdviceQuery),
    /// This is an instance (external) column queried at a certain relative location
    Instance(InstanceQuery),
    /// This is a negated polynomial
    Negated(Box<Expression<F>>),
    /// This is the sum of two polynomials
    Sum(Box<Expression<F>>, Box<Expression<F>>),
    /// This is the product of two polynomials
    Product(Box<Expression<F>>, Box<Expression<F>>),
    /// This is a scaled polynomial
    Scaled(Box<Expression<F>>, F),
}

impl<F: Field> Expression<F> {
    /// Evaluate the polynomial using the provided closures to perform the
    /// operations.
    #[allow(clippy::too_many_arguments)]
    pub fn evaluate<T>(
        &self,
        constant: &impl Fn(F) -> T,
        selector_column: &impl Fn(Selector) -> T,
        fixed_column: &impl Fn(FixedQuery) -> T,
        advice_column: &impl Fn(AdviceQuery) -> T,
        instance_column: &impl Fn(InstanceQuery) -> T,
        negated: &impl Fn(T) -> T,
        sum: &impl Fn(T, T) -> T,
        product: &impl Fn(T, T) -> T,
        scaled: &impl Fn(T, F) -> T,
    ) -> T {
        match self {
            Expression::Constant(scalar) => constant(*scalar),
            Expression::Selector(selector) => selector_column(*selector),
            Expression::Fixed(query) => fixed_column(*query),
            Expression::Advice(query) => advice_column(*query),
            Expression::Instance(query) => instance_column(*query),
            Expression::Negated(a) => {
                let a = a.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                negated(a)
            }
            Expression::Sum(a, b) => {
                let a = a.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                let b = b.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                sum(a, b)
            }
            Expression::Product(a, b) => {
                let a = a.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                let b = b.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                product(a, b)
            }
            Expression::Scaled(a, f) => {
                let a = a.evaluate(
                    constant,
                    selector_column,
                    fixed_column,
                    advice_column,
                    instance_column,
                    negated,
                    sum,
                    product,
                    scaled,
                );
                scaled(a, *f)
            }
        }
    }

    /// Compute the degree of this polynomial
    pub fn degree(&self) -> usize {
        match self {
            Expression::Constant(_) => 0,
            Expression::Selector(_) => 1,
            Expression::Fixed { .. } => 1,
            Expression::Advice { .. } => 1,
            Expression::Instance { .. } => 1,
            Expression::Negated(poly) => poly.degree(),
            Expression::Sum(a, b) => max(a.degree(), b.degree()),
            Expression::Product(a, b) => a.degree() + b.degree(),
            Expression::Scaled(poly, _) => poly.degree(),
        }
    }

    /// Square this expression.
    pub fn square(self) -> Self {
        self.clone() * self
    }

    /// Returns whether or not this expression contains a simple `Selector`.
    fn contains_simple_selector(&self) -> bool {
        self.evaluate(
            &|_| false,
            &|selector| selector.is_simple(),
            &|_| false,
            &|_| false,
            &|_| false,
            &|a| a,
            &|a, b| a || b,
            &|a, b| a || b,
            &|a, _| a,
        )
    }

    /// Extracts a simple selector from this gate, if present
    fn extract_simple_selector(&self) -> Option<Selector> {
        let op = |a, b| match (a, b) {
            (Some(a), None) | (None, Some(a)) => Some(a),
            (Some(_), Some(_)) => panic!("two simple selectors cannot be in the same expression"),
            _ => None,
        };

        self.evaluate(
            &|_| None,
            &|selector| {
                if selector.is_simple() {
                    Some(selector)
                } else {
                    None
                }
            },
            &|_| None,
            &|_| None,
            &|_| None,
            &|a| a,
            &op,
            &op,
            &|a, _| a,
        )
    }
}

impl<F: std::fmt::Debug> std::fmt::Debug for Expression<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Expression::Constant(scalar) => f.debug_tuple("Constant").field(scalar).finish(),
            Expression::Selector(selector) => f.debug_tuple("Selector").field(selector).finish(),
            // Skip enum variant and print query struct directly to maintain backwards compatibility.
            Expression::Fixed(FixedQuery {
                index,
                column_index,
                rotation,
            }) => f
                .debug_struct("Fixed")
                .field("query_index", index)
                .field("column_index", column_index)
                .field("rotation", rotation)
                .finish(),
            Expression::Advice(AdviceQuery {
                index,
                column_index,
                rotation,
            }) => f
                .debug_struct("Advice")
                .field("query_index", index)
                .field("column_index", column_index)
                .field("rotation", rotation)
                .finish(),
            Expression::Instance(InstanceQuery {
                index,
                column_index,
                rotation,
            }) => f
                .debug_struct("Instance")
                .field("query_index", index)
                .field("column_index", column_index)
                .field("rotation", rotation)
                .finish(),
            Expression::Negated(poly) => f.debug_tuple("Negated").field(poly).finish(),
            Expression::Sum(a, b) => f.debug_tuple("Sum").field(a).field(b).finish(),
            Expression::Product(a, b) => f.debug_tuple("Product").field(a).field(b).finish(),
            Expression::Scaled(poly, scalar) => {
                f.debug_tuple("Scaled").field(poly).field(scalar).finish()
            }
        }
    }
}

impl<F: Field> Neg for Expression<F> {
    type Output = Expression<F>;
    fn neg(self) -> Self::Output {
        Expression::Negated(Box::new(self))
    }
}

impl<F: Field> Add for Expression<F> {
    type Output = Expression<F>;
    fn add(self, rhs: Expression<F>) -> Expression<F> {
        if self.contains_simple_selector() || rhs.contains_simple_selector() {
            panic!("attempted to use a simple selector in an addition");
        }
        Expression::Sum(Box::new(self), Box::new(rhs))
    }
}

impl<F: Field> Sub for Expression<F> {
    type Output = Expression<F>;
    fn sub(self, rhs: Expression<F>) -> Expression<F> {
        if self.contains_simple_selector() || rhs.contains_simple_selector() {
            panic!("attempted to use a simple selector in a subtraction");
        }
        Expression::Sum(Box::new(self), Box::new(-rhs))
    }
}

impl<F: Field> Mul for Expression<F> {
    type Output = Expression<F>;
    fn mul(self, rhs: Expression<F>) -> Expression<F> {
        if self.contains_simple_selector() && rhs.contains_simple_selector() {
            panic!("attempted to multiply two expressions containing simple selectors");
        }
        Expression::Product(Box::new(self), Box::new(rhs))
    }
}

impl<F: Field> Mul<F> for Expression<F> {
    type Output = Expression<F>;
    fn mul(self, rhs: F) -> Expression<F> {
        Expression::Scaled(Box::new(self), rhs)
    }
}

/// Represents an index into a vector where each entry corresponds to a distinct
/// point that polynomials are queried at.
#[derive(Copy, Clone, Debug)]
pub(crate) struct PointIndex(pub usize);

/// A "virtual cell" is a PLONK cell that has been queried at a particular relative offset
/// within a custom gate.
#[derive(Clone, Debug)]
pub(crate) struct VirtualCell {
    pub(crate) column: Column<Any>,
    pub(crate) rotation: Rotation,
}

impl<Col: Into<Column<Any>>> From<(Col, Rotation)> for VirtualCell {
    fn from((column, rotation): (Col, Rotation)) -> Self {
        VirtualCell {
            column: column.into(),
            rotation,
        }
    }
}

/// An individual polynomial constraint.
///
/// These are returned by the closures passed to `ConstraintSystem::create_gate`.
#[derive(Debug)]
pub struct Constraint<F: Field> {
    name: &'static str,
    poly: Expression<F>,
}

impl<F: Field> From<Expression<F>> for Constraint<F> {
    fn from(poly: Expression<F>) -> Self {
        Constraint { name: "", poly }
    }
}

impl<F: Field> From<(&'static str, Expression<F>)> for Constraint<F> {
    fn from((name, poly): (&'static str, Expression<F>)) -> Self {
        Constraint { name, poly }
    }
}

impl<F: Field> From<Expression<F>> for Vec<Constraint<F>> {
    fn from(poly: Expression<F>) -> Self {
        vec![Constraint { name: "", poly }]
    }
}

/// A set of polynomial constraints with a common selector.
///
/// ```
/// use group::ff::Field;
/// use halo2_proofs::{pasta::Fp, plonk::{Constraints, Expression}, poly::Rotation};
/// # use halo2_proofs::plonk::ConstraintSystem;
///
/// # let mut meta = ConstraintSystem::<Fp>::default();
/// let a = meta.advice_column();
/// let b = meta.advice_column();
/// let c = meta.advice_column();
/// let s = meta.selector();
///
/// meta.create_gate("foo", |meta| {
///     let next = meta.query_advice(a, Rotation::next());
///     let a = meta.query_advice(a, Rotation::cur());
///     let b = meta.query_advice(b, Rotation::cur());
///     let c = meta.query_advice(c, Rotation::cur());
///     let s_ternary = meta.query_selector(s);
///
///     let one_minus_a = Expression::Constant(Fp::ONE) - a.clone();
///
///     Constraints::with_selector(
///         s_ternary,
///         std::array::IntoIter::new([
///             ("a is boolean", a.clone() * one_minus_a.clone()),
///             ("next == a ? b : c", next - (a * b + one_minus_a * c)),
///         ]),
///     )
/// });
/// ```
///
/// Note that the use of `std::array::IntoIter::new` is only necessary if you need to
/// support Rust 1.51 or 1.52. If your minimum supported Rust version is 1.53 or greater,
/// you can pass an array directly.
#[derive(Debug)]
pub struct Constraints<F: Field, C: Into<Constraint<F>>, Iter: IntoIterator<Item = C>> {
    selector: Expression<F>,
    constraints: Iter,
}

impl<F: Field, C: Into<Constraint<F>>, Iter: IntoIterator<Item = C>> Constraints<F, C, Iter> {
    /// Constructs a set of constraints that are controlled by the given selector.
    ///
    /// Each constraint `c` in `iterator` will be converted into the constraint
    /// `selector * c`.
    pub fn with_selector(selector: Expression<F>, constraints: Iter) -> Self {
        Constraints {
            selector,
            constraints,
        }
    }
}

fn apply_selector_to_constraint<F: Field, C: Into<Constraint<F>>>(
    (selector, c): (Expression<F>, C),
) -> Constraint<F> {
    let constraint: Constraint<F> = c.into();
    Constraint {
        name: constraint.name,
        poly: selector * constraint.poly,
    }
}

type ApplySelectorToConstraint<F, C> = fn((Expression<F>, C)) -> Constraint<F>;
type ConstraintsIterator<F, C, I> = std::iter::Map<
    std::iter::Zip<std::iter::Repeat<Expression<F>>, I>,
    ApplySelectorToConstraint<F, C>,
>;

impl<F: Field, C: Into<Constraint<F>>, Iter: IntoIterator<Item = C>> IntoIterator
    for Constraints<F, C, Iter>
{
    type Item = Constraint<F>;
    type IntoIter = ConstraintsIterator<F, C, Iter::IntoIter>;

    fn into_iter(self) -> Self::IntoIter {
        std::iter::repeat(self.selector)
            .zip(self.constraints.into_iter())
            .map(apply_selector_to_constraint)
    }
}

#[derive(Clone, Debug)]
pub(crate) struct Gate<F: Field> {
    name: &'static str,
    constraint_names: Vec<&'static str>,
    polys: Vec<Expression<F>>,
    /// We track queried selectors separately from other cells, so that we can use them to
    /// trigger debug checks on gates.
    queried_selectors: Vec<Selector>,
    queried_cells: Vec<VirtualCell>,
}

impl<F: Field> Gate<F> {
    pub(crate) fn name(&self) -> &'static str {
        self.name
    }

    pub(crate) fn constraint_name(&self, constraint_index: usize) -> &'static str {
        self.constraint_names[constraint_index]
    }

    pub(crate) fn polynomials(&self) -> &[Expression<F>] {
        &self.polys
    }

    pub(crate) fn queried_selectors(&self) -> &[Selector] {
        &self.queried_selectors
    }

    pub(crate) fn queried_cells(&self) -> &[VirtualCell] {
        &self.queried_cells
    }
}

/// This is a description of the circuit environment, such as the gate, column and
/// permutation arrangements.
#[derive(Debug, Clone)]
pub struct ConstraintSystem<F: Field> {
    pub(crate) num_fixed_columns: usize,
    pub(crate) num_advice_columns: usize,
    pub(crate) num_instance_columns: usize,
    pub(crate) num_selectors: usize,

    /// This is a cached vector that maps virtual selectors to the concrete
    /// fixed column that they were compressed into. This is just used by dev
    /// tooling right now.
    pub(crate) selector_map: Vec<Column<Fixed>>,

    pub(crate) gates: Vec<Gate<F>>,
    pub(crate) advice_queries: Vec<(Column<Advice>, Rotation)>,
    // Contains an integer for each advice column
    // identifying how many distinct queries it has
    // so far; should be same length as num_advice_columns.
    num_advice_queries: Vec<usize>,
    pub(crate) instance_queries: Vec<(Column<Instance>, Rotation)>,
    pub(crate) fixed_queries: Vec<(Column<Fixed>, Rotation)>,

    // Permutation argument for performing equality constraints
    pub(crate) permutation: permutation::Argument,

    // Vector of lookup arguments, where each corresponds to a sequence of
    // input expressions and a sequence of table expressions involved in the lookup.
    pub(crate) lookups: Vec<lookup::Argument<F>>,

    // Vector of fixed columns, which can be used to store constant values
    // that are copied into advice columns.
    pub(crate) constants: Vec<Column<Fixed>>,

    pub(crate) minimum_degree: Option<usize>,
}

/// Represents the minimal parameters that determine a `ConstraintSystem`.
#[allow(dead_code)]
#[derive(Debug)]
pub struct PinnedConstraintSystem<'a, F: Field> {
    num_fixed_columns: &'a usize,
    num_advice_columns: &'a usize,
    num_instance_columns: &'a usize,
    num_selectors: &'a usize,
    gates: PinnedGates<'a, F>,
    advice_queries: &'a Vec<(Column<Advice>, Rotation)>,
    instance_queries: &'a Vec<(Column<Instance>, Rotation)>,
    fixed_queries: &'a Vec<(Column<Fixed>, Rotation)>,
    permutation: &'a permutation::Argument,
    lookups: &'a Vec<lookup::Argument<F>>,
    constants: &'a Vec<Column<Fixed>>,
    minimum_degree: &'a Option<usize>,
}

struct PinnedGates<'a, F: Field>(&'a Vec<Gate<F>>);

impl<'a, F: Field> std::fmt::Debug for PinnedGates<'a, F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        f.debug_list()
            .entries(self.0.iter().flat_map(|gate| gate.polynomials().iter()))
            .finish()
    }
}

impl<F: Field> Default for ConstraintSystem<F> {
    fn default() -> ConstraintSystem<F> {
        ConstraintSystem {
            num_fixed_columns: 0,
            num_advice_columns: 0,
            num_instance_columns: 0,
            num_selectors: 0,
            selector_map: vec![],
            gates: vec![],
            fixed_queries: Vec::new(),
            advice_queries: Vec::new(),
            num_advice_queries: Vec::new(),
            instance_queries: Vec::new(),
            permutation: permutation::Argument::new(),
            lookups: Vec::new(),
            constants: vec![],
            minimum_degree: None,
        }
    }
}

impl<F: Field> ConstraintSystem<F> {
    /// Obtain a pinned version of this constraint system; a structure with the
    /// minimal parameters needed to determine the rest of the constraint
    /// system.
    pub fn pinned(&self) -> PinnedConstraintSystem<'_, F> {
        PinnedConstraintSystem {
            num_fixed_columns: &self.num_fixed_columns,
            num_advice_columns: &self.num_advice_columns,
            num_instance_columns: &self.num_instance_columns,
            num_selectors: &self.num_selectors,
            gates: PinnedGates(&self.gates),
            fixed_queries: &self.fixed_queries,
            advice_queries: &self.advice_queries,
            instance_queries: &self.instance_queries,
            permutation: &self.permutation,
            lookups: &self.lookups,
            constants: &self.constants,
            minimum_degree: &self.minimum_degree,
        }
    }

    /// Enables this fixed column to be used for global constant assignments.
    ///
    /// # Side-effects
    ///
    /// The column will be equality-enabled.
    pub fn enable_constant(&mut self, column: Column<Fixed>) {
        if !self.constants.contains(&column) {
            self.constants.push(column);
            self.enable_equality(column);
        }
    }

    /// Enable the ability to enforce equality over cells in this column
    pub fn enable_equality<C: Into<Column<Any>>>(&mut self, column: C) {
        let column = column.into();
        self.query_any_index(column, Rotation::cur());
        self.permutation.add_column(column);
    }

    /// Add a lookup argument for some input expressions and table columns.
    ///
    /// `table_map` returns a map between input expressions and the table columns
    /// they need to match.
    pub fn lookup(
        &mut self,
        table_map: impl FnOnce(&mut VirtualCells<'_, F>) -> Vec<(Expression<F>, TableColumn)>,
    ) -> usize {
        let mut cells = VirtualCells::new(self);
        let table_map = table_map(&mut cells)
            .into_iter()
            .map(|(input, table)| {
                if input.contains_simple_selector() {
                    panic!("expression containing simple selector supplied to lookup argument");
                }

                let table = cells.query_fixed(table.inner());

                (input, table)
            })
            .collect();

        let index = self.lookups.len();

        self.lookups.push(lookup::Argument::new(table_map));

        index
    }

    fn query_fixed_index(&mut self, column: Column<Fixed>) -> usize {
        // Return existing query, if it exists
        for (index, fixed_query) in self.fixed_queries.iter().enumerate() {
            if fixed_query == &(column, Rotation::cur()) {
                return index;
            }
        }

        // Make a new query
        let index = self.fixed_queries.len();
        self.fixed_queries.push((column, Rotation::cur()));

        index
    }

    pub(crate) fn query_advice_index(&mut self, column: Column<Advice>, at: Rotation) -> usize {
        // Return existing query, if it exists
        for (index, advice_query) in self.advice_queries.iter().enumerate() {
            if advice_query == &(column, at) {
                return index;
            }
        }

        // Make a new query
        let index = self.advice_queries.len();
        self.advice_queries.push((column, at));
        self.num_advice_queries[column.index] += 1;

        index
    }

    fn query_instance_index(&mut self, column: Column<Instance>, at: Rotation) -> usize {
        // Return existing query, if it exists
        for (index, instance_query) in self.instance_queries.iter().enumerate() {
            if instance_query == &(column, at) {
                return index;
            }
        }

        // Make a new query
        let index = self.instance_queries.len();
        self.instance_queries.push((column, at));

        index
    }

    fn query_any_index(&mut self, column: Column<Any>, at: Rotation) -> usize {
        match column.column_type() {
            Any::Advice => self.query_advice_index(Column::<Advice>::try_from(column).unwrap(), at),
            Any::Fixed => self.query_fixed_index(Column::<Fixed>::try_from(column).unwrap()),
            Any::Instance => {
                self.query_instance_index(Column::<Instance>::try_from(column).unwrap(), at)
            }
        }
    }

    pub(crate) fn get_advice_query_index(&self, column: Column<Advice>, at: Rotation) -> usize {
        for (index, advice_query) in self.advice_queries.iter().enumerate() {
            if advice_query == &(column, at) {
                return index;
            }
        }

        panic!("get_advice_query_index called for non-existent query");
    }

    pub(crate) fn get_fixed_query_index(&self, column: Column<Fixed>, at: Rotation) -> usize {
        for (index, fixed_query) in self.fixed_queries.iter().enumerate() {
            if fixed_query == &(column, at) {
                return index;
            }
        }

        panic!("get_fixed_query_index called for non-existent query");
    }

    pub(crate) fn get_instance_query_index(&self, column: Column<Instance>, at: Rotation) -> usize {
        for (index, instance_query) in self.instance_queries.iter().enumerate() {
            if instance_query == &(column, at) {
                return index;
            }
        }

        panic!("get_instance_query_index called for non-existent query");
    }

    pub(crate) fn get_any_query_index(&self, column: Column<Any>) -> usize {
        match column.column_type() {
            Any::Advice => self.get_advice_query_index(
                Column::<Advice>::try_from(column).unwrap(),
                Rotation::cur(),
            ),
            Any::Fixed => self
                .get_fixed_query_index(Column::<Fixed>::try_from(column).unwrap(), Rotation::cur()),
            Any::Instance => self.get_instance_query_index(
                Column::<Instance>::try_from(column).unwrap(),
                Rotation::cur(),
            ),
        }
    }

    /// Sets the minimum degree required by the circuit, which can be set to a
    /// larger amount than actually needed. This can be used, for example, to
    /// force the permutation argument to involve more columns in the same set.
    pub fn set_minimum_degree(&mut self, degree: usize) {
        self.minimum_degree = Some(degree);
    }

    /// Creates a new gate.
    ///
    /// # Panics
    ///
    /// A gate is required to contain polynomial constraints. This method will panic if
    /// `constraints` returns an empty iterator.
    pub fn create_gate<C: Into<Constraint<F>>, Iter: IntoIterator<Item = C>>(
        &mut self,
        name: &'static str,
        constraints: impl FnOnce(&mut VirtualCells<'_, F>) -> Iter,
    ) {
        let mut cells = VirtualCells::new(self);
        let constraints = constraints(&mut cells);
        let queried_selectors = cells.queried_selectors;
        let queried_cells = cells.queried_cells;

        let (constraint_names, polys): (_, Vec<_>) = constraints
            .into_iter()
            .map(|c| c.into())
            .map(|c| (c.name, c.poly))
            .unzip();

        assert!(
            !polys.is_empty(),
            "Gates must contain at least one constraint."
        );

        self.gates.push(Gate {
            name,
            constraint_names,
            polys,
            queried_selectors,
            queried_cells,
        });
    }

    /// This will compress selectors together depending on their provided
    /// assignments. This `ConstraintSystem` will then be modified to add new
    /// fixed columns (representing the actual selectors) and will return the
    /// polynomials for those columns. Finally, an internal map is updated to
    /// find which fixed column corresponds with a given `Selector`.
    ///
    /// Do not call this twice. Yes, this should be a builder pattern instead.
    pub(crate) fn compress_selectors(mut self, selectors: Vec<Vec<bool>>) -> (Self, Vec<Vec<F>>) {
        // The number of provided selector assignments must be the number we
        // counted for this constraint system.
        assert_eq!(selectors.len(), self.num_selectors);

        // Compute the maximal degree of every selector. We only consider the
        // expressions in gates, as lookup arguments cannot support simple
        // selectors. Selectors that are complex or do not appear in any gates
        // will have degree zero.
        let mut degrees = vec![0; selectors.len()];
        for expr in self.gates.iter().flat_map(|gate| gate.polys.iter()) {
            if let Some(selector) = expr.extract_simple_selector() {
                degrees[selector.0] = max(degrees[selector.0], expr.degree());
            }
        }

        // We will not increase the degree of the constraint system, so we limit
        // ourselves to the largest existing degree constraint.
        let max_degree = self.degree();

        let mut new_columns = vec![];
        let (polys, selector_assignment) = compress_selectors::process(
            selectors
                .into_iter()
                .zip(degrees.into_iter())
                .enumerate()
                .map(
                    |(i, (activations, max_degree))| compress_selectors::SelectorDescription {
                        selector: i,
                        activations,
                        max_degree,
                    },
                )
                .collect(),
            max_degree,
            || {
                let column = self.fixed_column();
                new_columns.push(column);
                Expression::Fixed(FixedQuery {
                    index: self.query_fixed_index(column),
                    column_index: column.index,
                    rotation: Rotation::cur(),
                })
            },
        );

        let mut selector_map = vec![None; selector_assignment.len()];
        let mut selector_replacements = vec![None; selector_assignment.len()];
        for assignment in selector_assignment {
            selector_replacements[assignment.selector] = Some(assignment.expression);
            selector_map[assignment.selector] = Some(new_columns[assignment.combination_index]);
        }

        self.selector_map = selector_map
            .into_iter()
            .map(|a| a.unwrap())
            .collect::<Vec<_>>();
        let selector_replacements = selector_replacements
            .into_iter()
            .map(|a| a.unwrap())
            .collect::<Vec<_>>();

        fn replace_selectors<F: Field>(
            expr: &mut Expression<F>,
            selector_replacements: &[Expression<F>],
            must_be_nonsimple: bool,
        ) {
            *expr = expr.evaluate(
                &|constant| Expression::Constant(constant),
                &|selector| {
                    if must_be_nonsimple {
                        // Simple selectors are prohibited from appearing in
                        // expressions in the lookup argument by
                        // `ConstraintSystem`.
                        assert!(!selector.is_simple());
                    }

                    selector_replacements[selector.0].clone()
                },
                &|query| Expression::Fixed(query),
                &|query| Expression::Advice(query),
                &|query| Expression::Instance(query),
                &|a| -a,
                &|a, b| a + b,
                &|a, b| a * b,
                &|a, f| a * f,
            );
        }

        // Substitute selectors for the real fixed columns in all gates
        for expr in self.gates.iter_mut().flat_map(|gate| gate.polys.iter_mut()) {
            replace_selectors(expr, &selector_replacements, false);
        }

        // Substitute non-simple selectors for the real fixed columns in all
        // lookup expressions
        for expr in self.lookups.iter_mut().flat_map(|lookup| {
            lookup
                .input_expressions
                .iter_mut()
                .chain(lookup.table_expressions.iter_mut())
        }) {
            replace_selectors(expr, &selector_replacements, true);
        }

        (self, polys)
    }

    /// Allocate a new (simple) selector. Simple selectors cannot be added to
    /// expressions nor multiplied by other expressions containing simple
    /// selectors. Also, simple selectors may not appear in lookup argument
    /// inputs.
    pub fn selector(&mut self) -> Selector {
        let index = self.num_selectors;
        self.num_selectors += 1;
        Selector(index, true)
    }

    /// Allocate a new complex selector that can appear anywhere
    /// within expressions.
    pub fn complex_selector(&mut self) -> Selector {
        let index = self.num_selectors;
        self.num_selectors += 1;
        Selector(index, false)
    }

    /// Allocates a new fixed column that can be used in a lookup table.
    pub fn lookup_table_column(&mut self) -> TableColumn {
        TableColumn {
            inner: self.fixed_column(),
        }
    }

    /// Allocate a new fixed column
    pub fn fixed_column(&mut self) -> Column<Fixed> {
        let tmp = Column {
            index: self.num_fixed_columns,
            column_type: Fixed,
        };
        self.num_fixed_columns += 1;
        tmp
    }

    /// Allocate a new advice column
    pub fn advice_column(&mut self) -> Column<Advice> {
        let tmp = Column {
            index: self.num_advice_columns,
            column_type: Advice,
        };
        self.num_advice_columns += 1;
        self.num_advice_queries.push(0);
        tmp
    }

    /// Allocate a new instance column
    pub fn instance_column(&mut self) -> Column<Instance> {
        let tmp = Column {
            index: self.num_instance_columns,
            column_type: Instance,
        };
        self.num_instance_columns += 1;
        tmp
    }

    /// Compute the degree of the constraint system (the maximum degree of all
    /// constraints).
    pub fn degree(&self) -> usize {
        // The permutation argument will serve alongside the gates, so must be
        // accounted for.
        let mut degree = self.permutation.required_degree();

        // The lookup argument also serves alongside the gates and must be accounted
        // for.
        degree = std::cmp::max(
            degree,
            self.lookups
                .iter()
                .map(|l| l.required_degree())
                .max()
                .unwrap_or(1),
        );

        // Account for each gate to ensure our quotient polynomial is the
        // correct degree and that our extended domain is the right size.
        degree = std::cmp::max(
            degree,
            self.gates
                .iter()
                .flat_map(|gate| gate.polynomials().iter().map(|poly| poly.degree()))
                .max()
                .unwrap_or(0),
        );

        std::cmp::max(degree, self.minimum_degree.unwrap_or(1))
    }

    /// Compute the number of blinding factors necessary to perfectly blind
    /// each of the prover's witness polynomials.
    pub fn blinding_factors(&self) -> usize {
        // All of the prover's advice columns are evaluated at no more than
        let factors = *self.num_advice_queries.iter().max().unwrap_or(&1);
        // distinct points during gate checks.

        // - The permutation argument witness polynomials are evaluated at most 3 times.
        // - Each lookup argument has independent witness polynomials, and they are
        //   evaluated at most 2 times.
        let factors = std::cmp::max(3, factors);

        // Each polynomial is evaluated at most an additional time during
        // multiopen (at x_3 to produce q_evals):
        let factors = factors + 1;

        // h(x) is derived by the other evaluations so it does not reveal
        // anything; in fact it does not even appear in the proof.

        // h(x_3) is also not revealed; the verifier only learns a single
        // evaluation of a polynomial in x_1 which has h(x_3) and another random
        // polynomial evaluated at x_3 as coefficients -- this random polynomial
        // is "random_poly" in the vanishing argument.

        // Add an additional blinding factor as a slight defense against
        // off-by-one errors.
        factors + 1
    }

    /// Returns the minimum necessary rows that need to exist in order to
    /// account for e.g. blinding factors.
    pub fn minimum_rows(&self) -> usize {
        self.blinding_factors() // m blinding factors
            + 1 // for l_{-(m + 1)} (l_last)
            + 1 // for l_0 (just for extra breathing room for the permutation
                // argument, to essentially force a separation in the
                // permutation polynomial between the roles of l_last, l_0
                // and the interstitial values.)
            + 1 // for at least one row
    }
}

/// Exposes the "virtual cells" that can be queried while creating a custom gate or lookup
/// table.
#[derive(Debug)]
pub struct VirtualCells<'a, F: Field> {
    meta: &'a mut ConstraintSystem<F>,
    queried_selectors: Vec<Selector>,
    queried_cells: Vec<VirtualCell>,
}

impl<'a, F: Field> VirtualCells<'a, F> {
    fn new(meta: &'a mut ConstraintSystem<F>) -> Self {
        VirtualCells {
            meta,
            queried_selectors: vec![],
            queried_cells: vec![],
        }
    }

    /// Query a selector at the current position.
    pub fn query_selector(&mut self, selector: Selector) -> Expression<F> {
        self.queried_selectors.push(selector);
        Expression::Selector(selector)
    }

    /// Query a fixed column at a relative position
    pub fn query_fixed(&mut self, column: Column<Fixed>) -> Expression<F> {
        self.queried_cells.push((column, Rotation::cur()).into());
        Expression::Fixed(FixedQuery {
            index: self.meta.query_fixed_index(column),
            column_index: column.index,
            rotation: Rotation::cur(),
        })
    }

    /// Query an advice column at a relative position
    pub fn query_advice(&mut self, column: Column<Advice>, at: Rotation) -> Expression<F> {
        self.queried_cells.push((column, at).into());
        Expression::Advice(AdviceQuery {
            index: self.meta.query_advice_index(column, at),
            column_index: column.index,
            rotation: at,
        })
    }

    /// Query an instance column at a relative position
    pub fn query_instance(&mut self, column: Column<Instance>, at: Rotation) -> Expression<F> {
        self.queried_cells.push((column, at).into());
        Expression::Instance(InstanceQuery {
            index: self.meta.query_instance_index(column, at),
            column_index: column.index,
            rotation: at,
        })
    }

    /// Query an Any column at a relative position
    ///
    /// # Panics
    ///
    /// Panics if query_fixed is called with a non-cur Rotation.
    pub fn query_any<C: Into<Column<Any>>>(&mut self, column: C, at: Rotation) -> Expression<F> {
        let column = column.into();
        match column.column_type() {
            Any::Advice => self.query_advice(Column::<Advice>::try_from(column).unwrap(), at),
            Any::Fixed => {
                if at != Rotation::cur() {
                    panic!("Fixed columns can only be queried at the current rotation");
                }
                self.query_fixed(Column::<Fixed>::try_from(column).unwrap())
            }
            Any::Instance => self.query_instance(Column::<Instance>::try_from(column).unwrap(), at),
        }
    }
}
