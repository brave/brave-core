#[macro_use]
extern crate criterion;

use group::ff::PrimeField;
use halo2_proofs::circuit::{Layouter, SimpleFloorPlanner, Value};
use halo2_proofs::dev::MockProver;
use halo2_proofs::plonk::*;
use halo2_proofs::poly::Rotation;
use pasta_curves::pallas;

use std::marker::PhantomData;

use criterion::{BenchmarkId, Criterion};

fn criterion_benchmark(c: &mut Criterion) {
    #[derive(Clone, Default)]
    struct MyCircuit<F: PrimeField> {
        _marker: PhantomData<F>,
    }

    #[derive(Clone)]
    struct MyConfig {
        selector: Selector,
        table: TableColumn,
        advice: Column<Advice>,
    }

    impl<F: PrimeField> Circuit<F> for MyCircuit<F> {
        type Config = MyConfig;
        type FloorPlanner = SimpleFloorPlanner;

        fn without_witnesses(&self) -> Self {
            Self::default()
        }

        fn configure(meta: &mut ConstraintSystem<F>) -> MyConfig {
            let config = MyConfig {
                selector: meta.complex_selector(),
                table: meta.lookup_table_column(),
                advice: meta.advice_column(),
            };

            meta.lookup(|meta| {
                let selector = meta.query_selector(config.selector);
                let not_selector = Expression::Constant(F::ONE) - selector.clone();
                let advice = meta.query_advice(config.advice, Rotation::cur());
                vec![(selector * advice + not_selector, config.table)]
            });

            config
        }

        fn synthesize(
            &self,
            config: MyConfig,
            mut layouter: impl Layouter<F>,
        ) -> Result<(), Error> {
            layouter.assign_table(
                || "8-bit table",
                |mut table| {
                    for row in 0u64..(1 << 8) {
                        table.assign_cell(
                            || format!("row {}", row),
                            config.table,
                            row as usize,
                            || Value::known(F::from(row + 1)),
                        )?;
                    }

                    Ok(())
                },
            )?;

            layouter.assign_region(
                || "assign values",
                |mut region| {
                    for offset in 0u64..(1 << 10) {
                        config.selector.enable(&mut region, offset as usize)?;
                        region.assign_advice(
                            || format!("offset {}", offset),
                            config.advice,
                            offset as usize,
                            || Value::known(F::from((offset % 256) + 1)),
                        )?;
                    }

                    Ok(())
                },
            )
        }
    }

    fn prover(k: u32) {
        let circuit = MyCircuit::<pallas::Base> {
            _marker: PhantomData,
        };
        let prover = MockProver::run(k, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()))
    }

    let k_range = 14..=18;

    let mut prover_group = c.benchmark_group("dev-lookup");
    prover_group.sample_size(10);
    for k in k_range {
        prover_group.bench_with_input(BenchmarkId::from_parameter(k), &k, |b, &k| {
            b.iter(|| prover(k));
        });
    }
    prover_group.finish();
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
