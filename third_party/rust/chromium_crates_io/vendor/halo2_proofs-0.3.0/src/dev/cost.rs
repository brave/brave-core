//! Developer tools for investigating the cost of a circuit.

use std::{
    cmp,
    collections::{HashMap, HashSet},
    iter,
    marker::PhantomData,
    ops::{Add, Mul},
};

use ff::{Field, PrimeField};
use group::prime::PrimeGroup;

use crate::{
    circuit::{layouter::RegionColumn, Value},
    plonk::{
        Advice, Any, Assigned, Assignment, Circuit, Column, ConstraintSystem, Error, Fixed,
        FloorPlanner, Instance, Selector,
    },
    poly::Rotation,
};

/// Measures a circuit to determine its costs, and explain what contributes to them.
#[allow(dead_code)]
#[derive(Debug)]
pub struct CircuitCost<G: PrimeGroup, ConcreteCircuit: Circuit<G::Scalar>> {
    /// Power-of-2 bound on the number of rows in the circuit.
    k: u32,
    /// Maximum degree of the circuit.
    max_deg: usize,
    /// Number of advice columns.
    advice_columns: usize,
    /// Number of direct queries for each column type.
    instance_queries: usize,
    advice_queries: usize,
    fixed_queries: usize,
    /// Number of lookup arguments.
    lookups: usize,
    /// Number of columns in the global permutation.
    permutation_cols: usize,
    /// Number of distinct sets of points in the multiopening argument.
    point_sets: usize,
    /// Maximum rows used over all columns
    max_rows: usize,
    /// Maximum rows used over all advice columns
    max_advice_rows: usize,
    /// Maximum rows used over all fixed columns
    max_fixed_rows: usize,
    num_fixed_columns: usize,
    num_advice_columns: usize,
    num_instance_columns: usize,
    num_total_columns: usize,

    _marker: PhantomData<(G, ConcreteCircuit)>,
}

/// Region implementation used by Layout
#[allow(dead_code)]
#[derive(Debug)]
pub(crate) struct LayoutRegion {
    /// The name of the region. Not required to be unique.
    pub(crate) name: String,
    /// The columns used by this region.
    pub(crate) columns: HashSet<RegionColumn>,
    /// The row that this region starts on, if known.
    pub(crate) offset: Option<usize>,
    /// The number of rows that this region takes up.
    pub(crate) rows: usize,
    /// The cells assigned in this region.
    pub(crate) cells: Vec<(RegionColumn, usize)>,
}

/// Cost and graphing layouter
#[derive(Default, Debug)]
pub(crate) struct Layout {
    /// k = 1 << n
    pub(crate) k: u32,
    /// Regions of the layout
    pub(crate) regions: Vec<LayoutRegion>,
    current_region: Option<usize>,
    /// Total row count
    pub(crate) total_rows: usize,
    /// Total advice rows
    pub(crate) total_advice_rows: usize,
    /// Total fixed rows
    pub(crate) total_fixed_rows: usize,
    /// Any cells assigned outside of a region.
    pub(crate) loose_cells: Vec<(RegionColumn, usize)>,
    /// Pairs of cells between which we have equality constraints.
    pub(crate) equality: Vec<(Column<Any>, usize, Column<Any>, usize)>,
    /// Selector assignments used for optimization pass
    pub(crate) selectors: Vec<Vec<bool>>,
}

impl Layout {
    /// Creates a empty layout
    pub fn new(k: u32, n: usize, num_selectors: usize) -> Self {
        Layout {
            k,
            regions: vec![],
            current_region: None,
            total_rows: 0,
            total_advice_rows: 0,
            total_fixed_rows: 0,
            /// Any cells assigned outside of a region.
            loose_cells: vec![],
            /// Pairs of cells between which we have equality constraints.
            equality: vec![],
            /// Selector assignments used for optimization pass
            selectors: vec![vec![false; n]; num_selectors],
        }
    }

    /// Update layout metadata
    pub fn update(&mut self, column: RegionColumn, row: usize) {
        self.total_rows = cmp::max(self.total_rows, row + 1);

        if let RegionColumn::Column(col) = column {
            match col.column_type() {
                Any::Advice => self.total_advice_rows = cmp::max(self.total_advice_rows, row + 1),
                Any::Fixed => self.total_fixed_rows = cmp::max(self.total_fixed_rows, row + 1),
                _ => {}
            }
        }

        if let Some(region) = self.current_region {
            let region = &mut self.regions[region];
            region.columns.insert(column);

            // The region offset is the earliest row assigned to.
            let mut offset = region.offset.unwrap_or(row);
            if row < offset {
                // The first row assigned was not at offset 0 within the region.
                region.rows += offset - row;
                offset = row;
            }
            // The number of rows in this region is the gap between the earliest and
            // latest rows assigned.
            region.rows = cmp::max(region.rows, row - offset + 1);
            region.offset = Some(offset);

            region.cells.push((column, row));
        } else {
            self.loose_cells.push((column, row));
        }
    }
}

impl<F: Field> Assignment<F> for Layout {
    fn enter_region<NR, N>(&mut self, name_fn: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR,
    {
        assert!(self.current_region.is_none());
        self.current_region = Some(self.regions.len());
        self.regions.push(LayoutRegion {
            name: name_fn().into(),
            columns: HashSet::default(),
            offset: None,
            rows: 0,
            cells: vec![],
        })
    }

    fn exit_region(&mut self) {
        assert!(self.current_region.is_some());
        self.current_region = None;
    }

    fn enable_selector<A, AR>(&mut self, _: A, selector: &Selector, row: usize) -> Result<(), Error>
    where
        A: FnOnce() -> AR,
        AR: Into<String>,
    {
        if let Some(cell) = self.selectors[selector.0].get_mut(row) {
            *cell = true;
        } else {
            return Err(Error::not_enough_rows_available(self.k));
        }

        self.update((*selector).into(), row);
        Ok(())
    }

    fn query_instance(&self, _: Column<Instance>, _: usize) -> Result<Value<F>, Error> {
        Ok(Value::unknown())
    }

    fn assign_advice<V, VR, A, AR>(
        &mut self,
        _: A,
        column: Column<Advice>,
        row: usize,
        _: V,
    ) -> Result<(), Error>
    where
        V: FnOnce() -> Value<VR>,
        VR: Into<Assigned<F>>,
        A: FnOnce() -> AR,
        AR: Into<String>,
    {
        self.update(Column::<Any>::from(column).into(), row);
        Ok(())
    }

    fn assign_fixed<V, VR, A, AR>(
        &mut self,
        _: A,
        column: Column<Fixed>,
        row: usize,
        _: V,
    ) -> Result<(), Error>
    where
        V: FnOnce() -> Value<VR>,
        VR: Into<Assigned<F>>,
        A: FnOnce() -> AR,
        AR: Into<String>,
    {
        self.update(Column::<Any>::from(column).into(), row);
        Ok(())
    }

    fn copy(
        &mut self,
        l_col: Column<Any>,
        l_row: usize,
        r_col: Column<Any>,
        r_row: usize,
    ) -> Result<(), crate::plonk::Error> {
        self.equality.push((l_col, l_row, r_col, r_row));
        Ok(())
    }

    fn fill_from_row(
        &mut self,
        _: Column<Fixed>,
        _: usize,
        _: Value<Assigned<F>>,
    ) -> Result<(), Error> {
        Ok(())
    }

    fn push_namespace<NR, N>(&mut self, _: N)
    where
        NR: Into<String>,
        N: FnOnce() -> NR,
    {
        // Do nothing; we don't care about namespaces in this context.
    }

    fn pop_namespace(&mut self, _: Option<String>) {
        // Do nothing; we don't care about namespaces in this context.
    }
}

impl<G: PrimeGroup, ConcreteCircuit: Circuit<G::Scalar>> CircuitCost<G, ConcreteCircuit> {
    /// Measures a circuit with parameter constant `k`.
    ///
    /// Panics if `k` is not large enough for the circuit.
    pub fn measure(k: u32, circuit: &ConcreteCircuit) -> Self {
        // Collect the layout details.
        let mut cs = ConstraintSystem::default();
        let config = ConcreteCircuit::configure(&mut cs);
        let mut layout = Layout::new(k, 1 << k, cs.num_selectors);
        ConcreteCircuit::FloorPlanner::synthesize(
            &mut layout,
            circuit,
            config,
            cs.constants.clone(),
        )
        .unwrap();
        let (cs, _) = cs.compress_selectors(layout.selectors);

        assert!((1 << k) >= cs.minimum_rows());

        // Figure out how many point sets we have due to queried cells.
        let mut column_queries: HashMap<Column<Any>, HashSet<i32>> = HashMap::new();
        for (c, r) in iter::empty()
            .chain(
                cs.advice_queries
                    .iter()
                    .map(|(c, r)| (Column::<Any>::from(*c), *r)),
            )
            .chain(cs.instance_queries.iter().map(|(c, r)| ((*c).into(), *r)))
            .chain(cs.fixed_queries.iter().map(|(c, r)| ((*c).into(), *r)))
            .chain(
                cs.permutation
                    .get_columns()
                    .into_iter()
                    .map(|c| (c, Rotation::cur())),
            )
        {
            column_queries.entry(c).or_default().insert(r.0);
        }
        let mut point_sets: HashSet<Vec<i32>> = HashSet::new();
        for (_, r) in column_queries {
            // Sort the query sets so we merge duplicates.
            let mut query_set: Vec<_> = r.into_iter().collect();
            query_set.sort_unstable();
            point_sets.insert(query_set);
        }

        // Include lookup polynomials in point sets:
        point_sets.insert(vec![0, 1]); // product_poly
        point_sets.insert(vec![-1, 0]); // permuted_input_poly
        point_sets.insert(vec![0]); // permuted_table_poly

        // Include permutation polynomials in point sets.
        point_sets.insert(vec![0, 1]); // permutation_product_poly
        let max_deg = cs.degree();
        let permutation_cols = cs.permutation.get_columns().len();
        if permutation_cols > max_deg - 2 {
            // permutation_product_poly for chaining chunks.
            point_sets.insert(vec![-((cs.blinding_factors() + 1) as i32), 0, 1]);
        }

        CircuitCost {
            k,
            max_deg,
            advice_columns: cs.num_advice_columns,
            instance_queries: cs.instance_queries.len(),
            advice_queries: cs.advice_queries.len(),
            fixed_queries: cs.fixed_queries.len(),
            lookups: cs.lookups.len(),
            permutation_cols,
            point_sets: point_sets.len(),
            _marker: PhantomData::default(),
            max_rows: layout.total_rows,
            max_advice_rows: layout.total_advice_rows,
            max_fixed_rows: layout.total_fixed_rows,
            num_advice_columns: cs.num_advice_columns,
            num_fixed_columns: cs.num_fixed_columns,
            num_instance_columns: cs.num_instance_columns,
            num_total_columns: cs.num_instance_columns
                + cs.num_advice_columns
                + cs.num_fixed_columns,
        }
    }

    fn permutation_chunks(&self) -> usize {
        let chunk_size = self.max_deg - 2;
        (self.permutation_cols + chunk_size - 1) / chunk_size
    }

    /// Returns the marginal proof size per instance of this circuit.
    pub fn marginal_proof_size(&self) -> MarginalProofSize<G> {
        let chunks = self.permutation_chunks();

        MarginalProofSize {
            // Cells:
            // - 1 commitment per advice column per instance
            // - 1 eval per instance column query per instance
            // - 1 eval per advice column query per instance
            instance: ProofContribution::new(0, self.instance_queries),
            advice: ProofContribution::new(self.advice_columns, self.advice_queries),

            // Lookup arguments:
            // - 3 commitments per lookup argument per instance
            // - 5 evals per lookup argument per instance
            lookups: ProofContribution::new(3 * self.lookups, 5 * self.lookups),

            // Global permutation argument:
            // - chunks commitments per instance
            // - 2 * chunks + (chunks - 1) evals per instance
            equality: ProofContribution::new(
                chunks,
                if chunks == 0 { chunks } else { 3 * chunks - 1 },
            ),

            _marker: PhantomData::default(),
        }
    }

    /// Returns the proof size for the given number of instances of this circuit.
    pub fn proof_size(&self, instances: usize) -> ProofSize<G> {
        let marginal = self.marginal_proof_size();

        ProofSize {
            // Cells:
            // - marginal cost per instance
            // - 1 eval per fixed column query
            instance: marginal.instance * instances,
            advice: marginal.advice * instances,
            fixed: ProofContribution::new(0, self.fixed_queries),

            // Lookup arguments:
            // - marginal cost per instance
            lookups: marginal.lookups * instances,

            // Global permutation argument:
            // - marginal cost per instance
            // - 1 eval per column
            equality: marginal.equality * instances
                + ProofContribution::new(0, self.permutation_cols),

            // Vanishing argument:
            // - 1 + (max_deg - 1) commitments
            // - 1 random_poly eval
            vanishing: ProofContribution::new(self.max_deg, 1),

            // Multiopening argument:
            // - f_commitment
            // - 1 eval per set of points in multiopen argument
            multiopen: ProofContribution::new(1, self.point_sets),

            // Polycommit:
            // - s_poly commitment
            // - inner product argument (2 * k round commitments)
            // - a
            // - xi
            polycomm: ProofContribution::new((1 + 2 * self.k).try_into().unwrap(), 2),

            _marker: PhantomData::default(),
        }
    }
}

/// (commitments, evaluations)
#[derive(Debug)]
struct ProofContribution {
    commitments: usize,
    evaluations: usize,
}

impl ProofContribution {
    fn new(commitments: usize, evaluations: usize) -> Self {
        ProofContribution {
            commitments,
            evaluations,
        }
    }

    fn len(&self, point: usize, scalar: usize) -> usize {
        self.commitments * point + self.evaluations * scalar
    }
}

impl Add for ProofContribution {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        Self {
            commitments: self.commitments + rhs.commitments,
            evaluations: self.evaluations + rhs.evaluations,
        }
    }
}

impl Mul<usize> for ProofContribution {
    type Output = Self;

    fn mul(self, instances: usize) -> Self::Output {
        Self {
            commitments: self.commitments * instances,
            evaluations: self.evaluations * instances,
        }
    }
}

/// The marginal size of a Halo 2 proof, broken down into its contributing factors.
#[derive(Debug)]
pub struct MarginalProofSize<G: PrimeGroup> {
    instance: ProofContribution,
    advice: ProofContribution,
    lookups: ProofContribution,
    equality: ProofContribution,
    _marker: PhantomData<G>,
}

impl<G: PrimeGroup> From<MarginalProofSize<G>> for usize {
    fn from(proof: MarginalProofSize<G>) -> Self {
        let point = G::Repr::default().as_ref().len();
        let scalar = <G::Scalar as PrimeField>::Repr::default().as_ref().len();

        proof.instance.len(point, scalar)
            + proof.advice.len(point, scalar)
            + proof.lookups.len(point, scalar)
            + proof.equality.len(point, scalar)
    }
}

/// The size of a Halo 2 proof, broken down into its contributing factors.
#[derive(Debug)]
pub struct ProofSize<G: PrimeGroup> {
    instance: ProofContribution,
    advice: ProofContribution,
    fixed: ProofContribution,
    lookups: ProofContribution,
    equality: ProofContribution,
    vanishing: ProofContribution,
    multiopen: ProofContribution,
    polycomm: ProofContribution,
    _marker: PhantomData<G>,
}

impl<G: PrimeGroup> From<ProofSize<G>> for usize {
    fn from(proof: ProofSize<G>) -> Self {
        let point = G::Repr::default().as_ref().len();
        let scalar = <G::Scalar as PrimeField>::Repr::default().as_ref().len();

        proof.instance.len(point, scalar)
            + proof.advice.len(point, scalar)
            + proof.fixed.len(point, scalar)
            + proof.lookups.len(point, scalar)
            + proof.equality.len(point, scalar)
            + proof.vanishing.len(point, scalar)
            + proof.multiopen.len(point, scalar)
            + proof.polycomm.len(point, scalar)
    }
}

#[cfg(test)]
mod tests {
    use pasta_curves::{Eq, Fp};

    use crate::circuit::SimpleFloorPlanner;

    use super::*;

    #[test]
    fn circuit_cost_without_permutation() {
        const K: u32 = 4;

        struct MyCircuit;
        impl Circuit<Fp> for MyCircuit {
            type Config = ();
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                Self
            }

            fn configure(_meta: &mut ConstraintSystem<Fp>) -> Self::Config {}

            fn synthesize(
                &self,
                _config: Self::Config,
                _layouter: impl crate::circuit::Layouter<Fp>,
            ) -> Result<(), Error> {
                Ok(())
            }
        }
        CircuitCost::<Eq, MyCircuit>::measure(K, &MyCircuit).proof_size(1);
    }
}
