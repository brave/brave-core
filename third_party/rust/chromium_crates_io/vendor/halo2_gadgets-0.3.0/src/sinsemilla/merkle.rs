//! Gadgets for implementing a Merkle tree with Sinsemilla.

use halo2_proofs::{
    circuit::{Chip, Layouter, Value},
    plonk::Error,
};
use pasta_curves::arithmetic::CurveAffine;

use super::{HashDomains, SinsemillaInstructions};

use crate::utilities::{cond_swap::CondSwapInstructions, i2lebsp, UtilitiesInstructions};

pub mod chip;

/// SWU hash-to-curve personalization for the Merkle CRH generator
pub const MERKLE_CRH_PERSONALIZATION: &str = "z.cash:Orchard-MerkleCRH";

/// Instructions to check the validity of a Merkle path of a given `PATH_LENGTH`.
/// The hash function used is a Sinsemilla instance with `K`-bit words.
/// The hash function can process `MAX_WORDS` words.
pub trait MerkleInstructions<
    C: CurveAffine,
    const PATH_LENGTH: usize,
    const K: usize,
    const MAX_WORDS: usize,
>:
    SinsemillaInstructions<C, K, MAX_WORDS>
    + CondSwapInstructions<C::Base>
    + UtilitiesInstructions<C::Base>
    + Chip<C::Base>
{
    /// Compute MerkleCRH for a given `layer`. The hash that computes the root
    /// is at layer 0, and the hashes that are applied to two leaves are at
    /// layer `MERKLE_DEPTH - 1` = layer 31.
    #[allow(non_snake_case)]
    fn hash_layer(
        &self,
        layouter: impl Layouter<C::Base>,
        Q: C,
        l: usize,
        left: Self::Var,
        right: Self::Var,
    ) -> Result<Self::Var, Error>;
}

/// Gadget representing a Merkle path that proves a leaf exists in a Merkle tree at a
/// specific position.
#[derive(Clone, Debug)]
pub struct MerklePath<
    C: CurveAffine,
    MerkleChip,
    const PATH_LENGTH: usize,
    const K: usize,
    const MAX_WORDS: usize,
    const PAR: usize,
> where
    MerkleChip: MerkleInstructions<C, PATH_LENGTH, K, MAX_WORDS> + Clone,
{
    chips: [MerkleChip; PAR],
    domain: MerkleChip::HashDomains,
    leaf_pos: Value<u32>,
    // The Merkle path is ordered from leaves to root.
    path: Value<[C::Base; PATH_LENGTH]>,
}

impl<
        C: CurveAffine,
        MerkleChip,
        const PATH_LENGTH: usize,
        const K: usize,
        const MAX_WORDS: usize,
        const PAR: usize,
    > MerklePath<C, MerkleChip, PATH_LENGTH, K, MAX_WORDS, PAR>
where
    MerkleChip: MerkleInstructions<C, PATH_LENGTH, K, MAX_WORDS> + Clone,
{
    /// Constructs a [`MerklePath`].
    ///
    /// A circuit may have many more columns available than are required by a single
    /// `MerkleChip`. To make better use of the available circuit area, the `MerklePath`
    /// gadget will distribute its path hashing across each `MerkleChip` in `chips`, such
    /// that each chip processes `ceil(PATH_LENGTH / PAR)` layers (with the last chip
    /// processing fewer layers if the division is inexact).
    pub fn construct(
        chips: [MerkleChip; PAR],
        domain: MerkleChip::HashDomains,
        leaf_pos: Value<u32>,
        path: Value<[C::Base; PATH_LENGTH]>,
    ) -> Self {
        assert_ne!(PAR, 0);
        Self {
            chips,
            domain,
            leaf_pos,
            path,
        }
    }
}

#[allow(non_snake_case)]
impl<
        C: CurveAffine,
        MerkleChip,
        const PATH_LENGTH: usize,
        const K: usize,
        const MAX_WORDS: usize,
        const PAR: usize,
    > MerklePath<C, MerkleChip, PATH_LENGTH, K, MAX_WORDS, PAR>
where
    MerkleChip: MerkleInstructions<C, PATH_LENGTH, K, MAX_WORDS> + Clone,
{
    /// Calculates the root of the tree containing the given leaf at this Merkle path.
    ///
    /// Implements [Zcash Protocol Specification Section 4.9: Merkle Path Validity][merklepath].
    ///
    /// [merklepath]: https://zips.z.cash/protocol/protocol.pdf#merklepath
    pub fn calculate_root(
        &self,
        mut layouter: impl Layouter<C::Base>,
        leaf: MerkleChip::Var,
    ) -> Result<MerkleChip::Var, Error> {
        // Each chip processes `ceil(PATH_LENGTH / PAR)` layers.
        let layers_per_chip = (PATH_LENGTH + PAR - 1) / PAR;

        // Assign each layer to a chip.
        let chips = (0..PATH_LENGTH).map(|i| self.chips[i / layers_per_chip].clone());

        // The Merkle path is ordered from leaves to root, which is consistent with the
        // little-endian representation of `pos` below.
        let path = self.path.transpose_array();

        // Get position as a PATH_LENGTH-bit bitstring (little-endian bit order).
        let pos: [Value<bool>; PATH_LENGTH] = {
            let pos: Value<[bool; PATH_LENGTH]> = self.leaf_pos.map(|pos| i2lebsp(pos as u64));
            pos.transpose_array()
        };

        let Q = self.domain.Q();

        let mut node = leaf;
        for (l, ((sibling, pos), chip)) in path.iter().zip(pos.iter()).zip(chips).enumerate() {
            // `l` = MERKLE_DEPTH - layer - 1, which is the index obtained from
            // enumerating this Merkle path (going from leaf to root).
            // For example, when `layer = 31` (the first sibling on the Merkle path),
            // we have `l` = 32 - 31 - 1 = 0.
            // On the other hand, when `layer = 0` (the final sibling on the Merkle path),
            // we have `l` = 32 - 0 - 1 = 31.

            // Constrain which of (node, sibling) is (left, right) with a conditional swap
            // tied to the current bit of the position.
            let pair = {
                let pair = (node, *sibling);

                // Swap node and sibling if needed
                chip.swap(layouter.namespace(|| "node position"), pair, *pos)?
            };

            // Compute the node in layer l from its children:
            //     M^l_i = MerkleCRH(l, M^{l+1}_{2i}, M^{l+1}_{2i+1})
            node = chip.hash_layer(
                layouter.namespace(|| format!("MerkleCRH({}, left, right)", l)),
                Q,
                l,
                pair.0,
                pair.1,
            )?;
        }

        Ok(node)
    }
}

#[cfg(test)]
pub mod tests {
    use super::{
        chip::{MerkleChip, MerkleConfig},
        MerklePath,
    };

    use crate::{
        ecc::tests::TestFixedBases,
        sinsemilla::{
            chip::SinsemillaChip,
            tests::{TestCommitDomain, TestHashDomain},
            HashDomains,
        },
        utilities::{i2lebsp, lookup_range_check::LookupRangeCheckConfig, UtilitiesInstructions},
    };

    use group::ff::{Field, PrimeField, PrimeFieldBits};
    use halo2_proofs::{
        circuit::{Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        pasta::pallas,
        plonk::{Circuit, ConstraintSystem, Error},
    };

    use rand::{rngs::OsRng, RngCore};
    use std::{convert::TryInto, iter};

    const MERKLE_DEPTH: usize = 32;

    #[derive(Default)]
    struct MyCircuit {
        leaf: Value<pallas::Base>,
        leaf_pos: Value<u32>,
        merkle_path: Value<[pallas::Base; MERKLE_DEPTH]>,
    }

    impl Circuit<pallas::Base> for MyCircuit {
        type Config = (
            MerkleConfig<TestHashDomain, TestCommitDomain, TestFixedBases>,
            MerkleConfig<TestHashDomain, TestCommitDomain, TestFixedBases>,
        );
        type FloorPlanner = SimpleFloorPlanner;

        fn without_witnesses(&self) -> Self {
            Self::default()
        }

        fn configure(meta: &mut ConstraintSystem<pallas::Base>) -> Self::Config {
            let advices = [
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
                meta.advice_column(),
            ];

            // Shared fixed column for loading constants
            let constants = meta.fixed_column();
            meta.enable_constant(constants);

            // NB: In the actual Action circuit, these fixed columns will be reused
            // by other chips. For this test, we are creating new fixed columns.
            let fixed_y_q_1 = meta.fixed_column();
            let fixed_y_q_2 = meta.fixed_column();

            // Fixed columns for the Sinsemilla generator lookup table
            let lookup = (
                meta.lookup_table_column(),
                meta.lookup_table_column(),
                meta.lookup_table_column(),
            );

            let range_check = LookupRangeCheckConfig::configure(meta, advices[9], lookup.0);

            let sinsemilla_config_1 = SinsemillaChip::configure(
                meta,
                advices[5..].try_into().unwrap(),
                advices[7],
                fixed_y_q_1,
                lookup,
                range_check,
            );
            let config1 = MerkleChip::configure(meta, sinsemilla_config_1);

            let sinsemilla_config_2 = SinsemillaChip::configure(
                meta,
                advices[..5].try_into().unwrap(),
                advices[2],
                fixed_y_q_2,
                lookup,
                range_check,
            );
            let config2 = MerkleChip::configure(meta, sinsemilla_config_2);

            (config1, config2)
        }

        fn synthesize(
            &self,
            config: Self::Config,
            mut layouter: impl Layouter<pallas::Base>,
        ) -> Result<(), Error> {
            // Load generator table (shared across both configs)
            SinsemillaChip::<TestHashDomain, TestCommitDomain, TestFixedBases>::load(
                config.0.sinsemilla_config.clone(),
                &mut layouter,
            )?;

            // Construct Merkle chips which will be placed side-by-side in the circuit.
            let chip_1 = MerkleChip::construct(config.0.clone());
            let chip_2 = MerkleChip::construct(config.1.clone());

            let leaf = chip_1.load_private(
                layouter.namespace(|| ""),
                config.0.cond_swap_config.a(),
                self.leaf,
            )?;

            let path = MerklePath {
                chips: [chip_1, chip_2],
                domain: TestHashDomain,
                leaf_pos: self.leaf_pos,
                path: self.merkle_path,
            };

            let computed_final_root =
                path.calculate_root(layouter.namespace(|| "calculate root"), leaf)?;

            self.leaf
                .zip(self.leaf_pos)
                .zip(self.merkle_path)
                .zip(computed_final_root.value())
                .assert_if_known(|(((leaf, leaf_pos), merkle_path), computed_final_root)| {
                    // The expected final root
                    let final_root =
                        merkle_path
                            .iter()
                            .enumerate()
                            .fold(*leaf, |node, (l, sibling)| {
                                let l = l as u8;
                                let (left, right) = if leaf_pos & (1 << l) == 0 {
                                    (&node, sibling)
                                } else {
                                    (sibling, &node)
                                };

                                use crate::sinsemilla::primitives as sinsemilla;
                                let merkle_crh =
                                    sinsemilla::HashDomain::from_Q(TestHashDomain.Q().into());

                                merkle_crh
                                    .hash(
                                        iter::empty()
                                            .chain(i2lebsp::<10>(l as u64).iter().copied())
                                            .chain(
                                                left.to_le_bits()
                                                    .iter()
                                                    .by_vals()
                                                    .take(pallas::Base::NUM_BITS as usize),
                                            )
                                            .chain(
                                                right
                                                    .to_le_bits()
                                                    .iter()
                                                    .by_vals()
                                                    .take(pallas::Base::NUM_BITS as usize),
                                            ),
                                    )
                                    .unwrap_or(pallas::Base::zero())
                            });

                    // Check the computed final root against the expected final root.
                    computed_final_root == &&final_root
                });

            Ok(())
        }
    }

    #[test]
    fn merkle_chip() {
        let mut rng = OsRng;

        // Choose a random leaf and position
        let leaf = pallas::Base::random(rng);
        let pos = rng.next_u32();

        // Choose a path of random inner nodes
        let path: Vec<_> = (0..(MERKLE_DEPTH))
            .map(|_| pallas::Base::random(rng))
            .collect();

        // The root is provided as a public input in the Orchard circuit.

        let circuit = MyCircuit {
            leaf: Value::known(leaf),
            leaf_pos: Value::known(pos),
            merkle_path: Value::known(path.try_into().unwrap()),
        };

        let prover = MockProver::run(11, &circuit, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()))
    }

    #[cfg(feature = "test-dev-graph")]
    #[test]
    fn print_merkle_chip() {
        use plotters::prelude::*;

        let root = BitMapBackend::new("merkle-path-layout.png", (1024, 7680)).into_drawing_area();
        root.fill(&WHITE).unwrap();
        let root = root.titled("MerkleCRH Path", ("sans-serif", 60)).unwrap();

        let circuit = MyCircuit::default();
        halo2_proofs::dev::CircuitLayout::default()
            .show_labels(false)
            .render(11, &circuit, &root)
            .unwrap();
    }
}
