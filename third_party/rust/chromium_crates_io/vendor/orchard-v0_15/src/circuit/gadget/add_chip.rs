use halo2_proofs::{
    circuit::{AssignedCell, Chip, Layouter},
    plonk::{self, Advice, Column, ConstraintSystem, Constraints, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

use super::AddInstruction;

/// Configuration for the addition chip.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "unstable-voting-circuits", visibility::make(pub))]
pub(in crate::circuit) struct AddConfig {
    a: Column<Advice>,
    b: Column<Advice>,
    c: Column<Advice>,
    q_add: Selector,
}

/// A chip implementing a single addition constraint `c = a + b` on a single row.
#[derive(Debug)]
#[cfg_attr(feature = "unstable-voting-circuits", visibility::make(pub))]
pub(in crate::circuit) struct AddChip {
    config: AddConfig,
}

impl Chip<pallas::Base> for AddChip {
    type Config = AddConfig;
    type Loaded = ();

    fn config(&self) -> &Self::Config {
        &self.config
    }

    fn loaded(&self) -> &Self::Loaded {
        &()
    }
}

impl AddChip {
    /// Configures the addition chip with the given advice columns.
    #[cfg_attr(feature = "unstable-voting-circuits", visibility::make(pub))]
    pub(in crate::circuit) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        a: Column<Advice>,
        b: Column<Advice>,
        c: Column<Advice>,
    ) -> AddConfig {
        let q_add = meta.selector();
        meta.create_gate("Field element addition: c = a + b", |meta| {
            let q_add = meta.query_selector(q_add);
            let a = meta.query_advice(a, Rotation::cur());
            let b = meta.query_advice(b, Rotation::cur());
            let c = meta.query_advice(c, Rotation::cur());

            Constraints::with_selector(q_add, Some(a + b - c))
        });

        AddConfig { a, b, c, q_add }
    }

    /// Constructs an addition chip from the given config.
    #[cfg_attr(feature = "unstable-voting-circuits", visibility::make(pub))]
    pub(in crate::circuit) fn construct(config: AddConfig) -> Self {
        Self { config }
    }
}

impl AddInstruction<pallas::Base> for AddChip {
    fn add(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        a: &AssignedCell<pallas::Base, pallas::Base>,
        b: &AssignedCell<pallas::Base, pallas::Base>,
    ) -> Result<AssignedCell<pallas::Base, pallas::Base>, plonk::Error> {
        layouter.assign_region(
            || "c = a + b",
            |mut region| {
                self.config.q_add.enable(&mut region, 0)?;

                a.copy_advice(|| "copy a", &mut region, self.config.a, 0)?;
                b.copy_advice(|| "copy b", &mut region, self.config.b, 0)?;

                let scalar_val = a.value().zip(b.value()).map(|(a, b)| a + b);
                region.assign_advice(|| "c", self.config.c, 0, || scalar_val)
            },
        )
    }
}
