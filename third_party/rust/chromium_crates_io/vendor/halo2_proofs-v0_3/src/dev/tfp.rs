use std::{fmt, marker::PhantomData};

use ff::Field;
use tracing::{debug, debug_span, span::EnteredSpan};

use crate::{
    circuit::{layouter::RegionLayouter, AssignedCell, Cell, Layouter, Region, Table, Value},
    plonk::{
        Advice, Any, Assigned, Assignment, Circuit, Column, ConstraintSystem, Error, Fixed,
        FloorPlanner, Instance, Selector,
    },
};

/// A helper type that augments a [`FloorPlanner`] with [`tracing`] spans and events.
///
/// `TracingFloorPlanner` can be used to instrument your circuit and determine exactly
/// what is happening during a particular run of keygen or proving. This can be useful for
/// identifying unexpected non-determinism or changes to a circuit.
///
/// # No stability guarantees
///
/// The `tracing` output is intended for use during circuit development. It should not be
/// considered production-stable, and the precise format or data exposed may change at any
/// time.
///
/// # Examples
///
/// ```
/// use ff::Field;
/// use halo2_proofs::{
///     circuit::{floor_planner, Layouter, Value},
///     dev::TracingFloorPlanner,
///     plonk::{Circuit, ConstraintSystem, Error},
/// };
///
/// # struct MyCircuit<F: Field> {
/// #     some_witness: Value<F>,
/// # };
/// # #[derive(Clone)]
/// # struct MyConfig;
/// impl<F: Field> Circuit<F> for MyCircuit<F> {
///     // Wrap `TracingFloorPlanner` around your existing floor planner of choice.
///     //type FloorPlanner = floor_planner::V1;
///     type FloorPlanner = TracingFloorPlanner<floor_planner::V1>;
///
///     // The rest of your `Circuit` implementation is unchanged.
///     type Config = MyConfig;
///
///     fn without_witnesses(&self) -> Self {
///         Self { some_witness: Value::unknown() }
///     }
///
///     fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
///         // ..
/// #       todo!()
///     }
///
///     fn synthesize(&self, config: Self::Config, layouter: impl Layouter<F>) -> Result<(), Error> {
///         // ..
/// #       todo!()
///     }
/// }
///
/// #[test]
/// fn some_circuit_test() {
///     // At the start of your test, enable tracing.
///     tracing_subscriber::fmt()
///         .with_max_level(tracing::Level::DEBUG)
///         .with_ansi(false)
///         .without_time()
///         .init();
///
///     // Now when the rest of the test runs, you will get `tracing` output for every
///     // operation that the circuit performs under the hood!
/// }
/// ```
#[derive(Debug)]
pub struct TracingFloorPlanner<P: FloorPlanner> {
    _phantom: PhantomData<P>,
}

impl<P: FloorPlanner> FloorPlanner for TracingFloorPlanner<P> {
    fn synthesize<F: Field, CS: Assignment<F>, C: Circuit<F>>(
        cs: &mut CS,
        circuit: &C,
        config: C::Config,
        constants: Vec<Column<Fixed>>,
    ) -> Result<(), Error> {
        P::synthesize(
            &mut TracingAssignment::new(cs),
            &TracingCircuit::borrowed(circuit),
            config,
            constants,
        )
    }
}

/// A helper type that augments a [`Circuit`] with [`tracing`] spans and events.
enum TracingCircuit<'c, F: Field, C: Circuit<F>> {
    Borrowed(&'c C, PhantomData<F>),
    Owned(C, PhantomData<F>),
}

impl<'c, F: Field, C: Circuit<F>> TracingCircuit<'c, F, C> {
    fn borrowed(circuit: &'c C) -> Self {
        Self::Borrowed(circuit, PhantomData)
    }

    fn owned(circuit: C) -> Self {
        Self::Owned(circuit, PhantomData)
    }

    fn inner_ref(&self) -> &C {
        match self {
            TracingCircuit::Borrowed(circuit, ..) => circuit,
            TracingCircuit::Owned(circuit, ..) => circuit,
        }
    }
}

impl<'c, F: Field, C: Circuit<F>> Circuit<F> for TracingCircuit<'c, F, C> {
    type Config = C::Config;
    type FloorPlanner = C::FloorPlanner;

    fn without_witnesses(&self) -> Self {
        Self::owned(self.inner_ref().without_witnesses())
    }

    fn configure(meta: &mut ConstraintSystem<F>) -> Self::Config {
        let _span = debug_span!("configure").entered();
        C::configure(meta)
    }

    fn synthesize(&self, config: Self::Config, layouter: impl Layouter<F>) -> Result<(), Error> {
        let _span = debug_span!("synthesize").entered();
        self.inner_ref()
            .synthesize(config, TracingLayouter::new(layouter))
    }
}

/// A helper type that augments a [`Layouter`] with [`tracing`] spans and events.
struct TracingLayouter<F: Field, L: Layouter<F>> {
    layouter: L,
    namespace_spans: Vec<EnteredSpan>,
    _phantom: PhantomData<F>,
}

impl<F: Field, L: Layouter<F>> TracingLayouter<F, L> {
    fn new(layouter: L) -> Self {
        Self {
            layouter,
            namespace_spans: vec![],
            _phantom: PhantomData,
        }
    }
}

impl<F: Field, L: Layouter<F>> Layouter<F> for TracingLayouter<F, L> {
    type Root = Self;

    fn assign_region<A, AR, N, NR>(&mut self, name: N, mut assignment: A) -> Result<AR, Error>
    where
        A: FnMut(Region<'_, F>) -> Result<AR, Error>,
        N: Fn() -> NR,
        NR: Into<String>,
    {
        let _span = debug_span!("region", name = name().into()).entered();
        self.layouter.assign_region(name, |region| {
            let mut region = TracingRegion(region);
            let region: &mut dyn RegionLayouter<F> = &mut region;
            assignment(region.into())
        })
    }

    fn assign_table<A, N, NR>(&mut self, name: N, assignment: A) -> Result<(), Error>
    where
        A: FnMut(Table<'_, F>) -> Result<(), Error>,
        N: Fn() -> NR,
        NR: Into<String>,
    {
        let _span = debug_span!("table", name = name().into()).entered();
        self.layouter.assign_table(name, assignment)
    }

    fn constrain_instance(
        &mut self,
        cell: Cell,
        column: Column<Instance>,
        row: usize,
    ) -> Result<(), Error> {
        self.layouter.constrain_instance(cell, column, row)
    }

    fn get_root(&mut self) -> &mut Self::Root {
        self
    }

    fn push_namespace<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR,
    {
        let name = name_fn().into();
        self.namespace_spans.push(debug_span!("ns", name).entered());
        self.layouter.push_namespace(|| name);
    }

    fn pop_namespace(&mut self, gadget_name: Option<String>) {
        self.layouter.pop_namespace(gadget_name);
        self.namespace_spans.pop();
    }
}

fn debug_value_and_return_cell<F: Field, V: fmt::Debug>(value: AssignedCell<V, F>) -> Cell {
    if let Some(v) = value.value().into_option() {
        debug!(target: "assigned", value = ?v);
    }
    value.cell()
}

/// A helper type that augments a [`Region`] with [`tracing`] spans and events.
#[derive(Debug)]
struct TracingRegion<'r, F: Field>(Region<'r, F>);

impl<'r, F: Field> RegionLayouter<F> for TracingRegion<'r, F> {
    fn enable_selector<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        selector: &Selector,
        offset: usize,
    ) -> Result<(), Error> {
        let _guard = debug_span!("enable_selector", name = annotation(), offset = offset).entered();
        debug!(target: "layouter", "Entered");
        self.0.enable_selector(annotation, selector, offset)
    }

    fn assign_advice<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error> {
        let _guard =
            debug_span!("assign_advice", name = annotation(), column = ?column, offset = offset)
                .entered();
        debug!(target: "layouter", "Entered");
        self.0
            .assign_advice(annotation, column, offset, to)
            .map(debug_value_and_return_cell)
    }

    fn assign_advice_from_constant<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Advice>,
        offset: usize,
        constant: Assigned<F>,
    ) -> Result<Cell, Error> {
        let _guard = debug_span!("assign_advice_from_constant",
            name = annotation(),
            column = ?column,
            offset = offset,
            constant = ?constant,
        )
        .entered();
        debug!(target: "layouter", "Entered");
        self.0
            .assign_advice_from_constant(annotation, column, offset, constant)
            .map(debug_value_and_return_cell)
    }

    fn assign_advice_from_instance<'v>(
        &mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        instance: Column<Instance>,
        row: usize,
        advice: Column<Advice>,
        offset: usize,
    ) -> Result<(Cell, Value<F>), Error> {
        let _guard = debug_span!("assign_advice_from_instance",
            name = annotation(),
            instance = ?instance,
            row = row,
            advice = ?advice,
            offset = offset,
        )
        .entered();
        debug!(target: "layouter", "Entered");
        self.0
            .assign_advice_from_instance(annotation, instance, row, advice, offset)
            .map(|value| {
                if let Some(v) = value.value().into_option() {
                    debug!(target: "assigned", value = ?v);
                }
                (value.cell(), value.value().cloned())
            })
    }

    fn instance_value(
        &mut self,
        instance: Column<Instance>,
        row: usize,
    ) -> Result<Value<F>, Error> {
        self.0.instance_value(instance, row)
    }

    fn assign_fixed<'v>(
        &'v mut self,
        annotation: &'v (dyn Fn() -> String + 'v),
        column: Column<Fixed>,
        offset: usize,
        to: &'v mut (dyn FnMut() -> Value<Assigned<F>> + 'v),
    ) -> Result<Cell, Error> {
        let _guard =
            debug_span!("assign_fixed", name = annotation(), column = ?column, offset = offset)
                .entered();
        debug!(target: "layouter", "Entered");
        self.0
            .assign_fixed(annotation, column, offset, to)
            .map(debug_value_and_return_cell)
    }

    fn constrain_constant(&mut self, cell: Cell, constant: Assigned<F>) -> Result<(), Error> {
        debug!(target: "constrain_constant", cell = ?cell, constant = ?constant);
        self.0.constrain_constant(cell, constant)
    }

    fn constrain_equal(&mut self, left: Cell, right: Cell) -> Result<(), Error> {
        debug!(target: "constrain_equal", left = ?left, right = ?right);
        self.0.constrain_equal(left, right)
    }
}

/// A helper type that augments an [`Assignment`] with [`tracing`] spans and events.
struct TracingAssignment<'cs, F: Field, CS: Assignment<F>> {
    cs: &'cs mut CS,
    in_region: bool,
    _phantom: PhantomData<F>,
}

impl<'cs, F: Field, CS: Assignment<F>> TracingAssignment<'cs, F, CS> {
    fn new(cs: &'cs mut CS) -> Self {
        Self {
            cs,
            in_region: false,
            _phantom: PhantomData,
        }
    }
}

impl<'cs, F: Field, CS: Assignment<F>> Assignment<F> for TracingAssignment<'cs, F, CS> {
    fn enter_region<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR,
    {
        self.in_region = true;
        self.cs.enter_region(name_fn);
    }

    fn exit_region(&mut self) {
        self.cs.exit_region();
        self.in_region = false;
    }

    fn enable_selector<A, AR>(
        &mut self,
        annotation: A,
        selector: &Selector,
        row: usize,
    ) -> Result<(), Error>
    where
        A: FnOnce() -> AR,
        AR: Into<String>,
    {
        let annotation = annotation().into();
        if self.in_region {
            debug!(target: "position", row = row);
        } else {
            debug!(target: "enable_selector", name = annotation, row = row);
        }
        self.cs.enable_selector(|| annotation, selector, row)
    }

    fn query_instance(&self, column: Column<Instance>, row: usize) -> Result<Value<F>, Error> {
        let _guard = debug_span!("positioned").entered();
        debug!(target: "query_instance", column = ?column, row = row);
        self.cs.query_instance(column, row)
    }

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
        AR: Into<String>,
    {
        let annotation = annotation().into();
        if self.in_region {
            debug!(target: "position", row = row);
        } else {
            debug!(target: "assign_advice", name = annotation, column = ?column, row = row);
        }
        self.cs.assign_advice(|| annotation, column, row, to)
    }

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
        AR: Into<String>,
    {
        let annotation = annotation().into();
        if self.in_region {
            debug!(target: "position", row = row);
        } else {
            debug!(target: "assign_fixed", name = annotation, column = ?column, row = row);
        }
        self.cs.assign_fixed(|| annotation, column, row, to)
    }

    fn copy(
        &mut self,
        left_column: Column<Any>,
        left_row: usize,
        right_column: Column<Any>,
        right_row: usize,
    ) -> Result<(), Error> {
        let _guard = debug_span!("positioned").entered();
        debug!(
            target: "copy",
            left_column = ?left_column,
            left_row = left_row,
            right_column = ?right_column,
            right_row = right_row,
        );
        self.cs.copy(left_column, left_row, right_column, right_row)
    }

    fn fill_from_row(
        &mut self,
        column: Column<Fixed>,
        row: usize,
        to: Value<Assigned<F>>,
    ) -> Result<(), Error> {
        let _guard = debug_span!("positioned").entered();
        debug!(target: "fill_from_row", column = ?column, row = row);
        self.cs.fill_from_row(column, row, to)
    }

    fn push_namespace<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR,
    {
        // We enter namespace spans in TracingLayouter.
        self.cs.push_namespace(name_fn)
    }

    fn pop_namespace(&mut self, gadget_name: Option<String>) {
        self.cs.pop_namespace(gadget_name);
        // We exit namespace spans in TracingLayouter.
    }
}
