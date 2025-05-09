use core::iter;

use group::ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Value},
    plonk::{Advice, Column, ConstraintSystem, Constraints, Error, Expression, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

use crate::constants::{OrchardCommitDomains, OrchardFixedBases, OrchardHashDomains, T_P};
use halo2_gadgets::{
    ecc::{chip::EccChip, ScalarFixed, X},
    sinsemilla::{chip::SinsemillaChip, CommitDomain, Message, MessagePiece},
    utilities::{bool_check, RangeConstrained},
};

#[derive(Clone, Debug)]
pub struct CommitIvkConfig {
    q_commit_ivk: Selector,
    advices: [Column<Advice>; 10],
}

#[derive(Clone, Debug)]
pub struct CommitIvkChip {
    config: CommitIvkConfig,
}

impl CommitIvkChip {
    pub(in crate::circuit) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        advices: [Column<Advice>; 10],
    ) -> CommitIvkConfig {
        let q_commit_ivk = meta.selector();

        let config = CommitIvkConfig {
            q_commit_ivk,
            advices,
        };

        // <https://zips.z.cash/protocol/nu5.pdf#concretesinsemillacommit>
        // We need to hash `ak || nk` where each of `ak`, `nk` is a field element (255 bits).
        //
        // a = bits 0..=249 of `ak`
        // b = b_0||b_1||b_2`
        //   = (bits 250..=253 of `ak`) || (bit 254 of  `ak`) || (bits 0..=4 of  `nk`)
        // c = bits 5..=244 of `nk`
        // d = d_0||d_1` = (bits 245..=253 of `nk`) || (bit 254 of `nk`)
        //
        // `a`, `b`, `c`, `d` have been constrained by the Sinsemilla hash to be:
        //   - a: 250 bits,
        //   - b: 10 bits,
        //   - c: 240 bits,
        //   - d: 10 bits
        //
        // https://p.z.cash/orchard-0.1:commit-ivk-decompositions
        // https://p.z.cash/orchard-0.1:commit-ivk-region-layout?partial
        /*
            The pieces are laid out in this configuration:

            |  A_0  |  A_1  |  A_2  |  A_3  |  A_4  |  A_5  |  A_6  |    A_7    |       A_8      | q_commit_ivk |
            -----------------------------------------------------------------------------------------------------
            |   ak  |   a   |   b   |  b_0  |  b_1  |  b_2  | z13_a |  a_prime  |   z13_a_prime  |       1      |
            |   nk  |   c   |   d   |  d_0  |  d_1  |       | z13_c | b2_c_prime| z14_b2_c_prime |       0      |

        */
        meta.create_gate("CommitIvk canonicity check", |meta| {
            let q_commit_ivk = meta.query_selector(config.q_commit_ivk);

            // Useful constants
            let two_pow_4 = pallas::Base::from(1 << 4);
            let two_pow_5 = pallas::Base::from(1 << 5);
            let two_pow_9 = two_pow_4 * two_pow_5;
            let two_pow_250 = pallas::Base::from_u128(1 << 125).square();
            let two_pow_254 = two_pow_250 * two_pow_4;

            let ak = meta.query_advice(config.advices[0], Rotation::cur());
            let nk = meta.query_advice(config.advices[0], Rotation::next());

            // `a` is constrained by the Sinsemilla hash to be 250 bits.
            let a = meta.query_advice(config.advices[1], Rotation::cur());
            // `b` is constrained by the Sinsemilla hash to be 10 bits.
            let b_whole = meta.query_advice(config.advices[2], Rotation::cur());
            // `c` is constrained by the Sinsemilla hash to be 240 bits.
            let c = meta.query_advice(config.advices[1], Rotation::next());
            // `d` is constrained by the Sinsemilla hash to be 10 bits.
            let d_whole = meta.query_advice(config.advices[2], Rotation::next());

            // b = b_0||b_1||b_2`
            //   = (bits 250..=253 of `ak`) || (bit 254 of  `ak`) || (bits 0..=4 of  `nk`)
            //
            // b_0 has been constrained outside this gate to be a four-bit value.
            let b_0 = meta.query_advice(config.advices[3], Rotation::cur());
            // This gate constrains b_1 to be a one-bit value.
            let b_1 = meta.query_advice(config.advices[4], Rotation::cur());
            // b_2 has been constrained outside this gate to be a five-bit value.
            let b_2 = meta.query_advice(config.advices[5], Rotation::cur());
            // Check that b_whole is consistent with the witnessed subpieces.
            let b_decomposition_check =
                b_whole - (b_0.clone() + b_1.clone() * two_pow_4 + b_2.clone() * two_pow_5);

            // d = d_0||d_1` = (bits 245..=253 of `nk`) || (bit 254 of `nk`)
            //
            // d_0 has been constrained outside this gate to be a nine-bit value.
            let d_0 = meta.query_advice(config.advices[3], Rotation::next());
            // This gate constrains d_1 to be a one-bit value.
            let d_1 = meta.query_advice(config.advices[4], Rotation::next());
            // Check that d_whole is consistent with the witnessed subpieces.
            let d_decomposition_check = d_whole - (d_0.clone() + d_1.clone() * two_pow_9);

            // Check `b_1` and `d_1` are each a single-bit value.
            // https://p.z.cash/orchard-0.1:commit-ivk-bit-lengths?partial
            let b1_bool_check = bool_check(b_1.clone());
            let d1_bool_check = bool_check(d_1.clone());

            // Check that ak = a (250 bits) || b_0 (4 bits) || b_1 (1 bit)
            let ak_decomposition_check =
                a.clone() + b_0.clone() * two_pow_250 + b_1.clone() * two_pow_254 - ak;

            // Check that nk = b_2 (5 bits) || c (240 bits) || d_0 (9 bits) || d_1 (1 bit)
            let nk_decomposition_check = {
                let two_pow_245 = pallas::Base::from(1 << 49).pow([5, 0, 0, 0]);

                b_2.clone()
                    + c.clone() * two_pow_5
                    + d_0.clone() * two_pow_245
                    + d_1.clone() * two_pow_254
                    - nk
            };

            // ak = a (250 bits) || b_0 (4 bits) || b_1 (1 bit)
            // The `ak` canonicity checks are enforced if and only if `b_1` = 1.
            // https://p.z.cash/orchard-0.1:commit-ivk-canonicity-ak?partial
            let ak_canonicity_checks = {
                // b_1 = 1 => b_0 = 0
                let b0_canon_check = b_1.clone() * b_0;

                // z13_a is the 13th running sum output by the 10-bit Sinsemilla decomposition of `a`.
                // b_1 = 1 => z13_a = 0
                let z13_a_check = {
                    let z13_a = meta.query_advice(config.advices[6], Rotation::cur());
                    b_1.clone() * z13_a
                };

                // Check that a_prime = a + 2^130 - t_P.
                // This is checked regardless of the value of b_1.
                let a_prime_check = {
                    let a_prime = meta.query_advice(config.advices[7], Rotation::cur());
                    let two_pow_130 =
                        Expression::Constant(pallas::Base::from_u128(1 << 65).square());
                    let t_p = Expression::Constant(pallas::Base::from_u128(T_P));
                    a + two_pow_130 - t_p - a_prime
                };

                // Check that the running sum output by the 130-bit little-endian decomposition of
                // `a_prime` is zero.
                let z13_a_prime = {
                    let z13_a_prime = meta.query_advice(config.advices[8], Rotation::cur());
                    b_1 * z13_a_prime
                };

                iter::empty()
                    .chain(Some(("b0_canon_check", b0_canon_check)))
                    .chain(Some(("z13_a_check", z13_a_check)))
                    .chain(Some(("a_prime_check", a_prime_check)))
                    .chain(Some(("z13_a_prime", z13_a_prime)))
            };

            // nk = b_2 (5 bits) || c (240 bits) || d_0 (9 bits) || d_1 (1 bit)
            // The `nk` canonicity checks are enforced if and only if `d_1` = 1.
            // https://p.z.cash/orchard-0.1:commit-ivk-canonicity-nk?partial
            let nk_canonicity_checks = {
                // d_1 = 1 => d_0 = 0
                let c0_canon_check = d_1.clone() * d_0;

                // d_1 = 1 => z13_c = 0, where z13_c is the 13th running sum
                // output by the 10-bit Sinsemilla decomposition of `c`.
                let z13_c_check = {
                    let z13_c = meta.query_advice(config.advices[6], Rotation::next());
                    d_1.clone() * z13_c
                };

                // Check that b2_c_prime = b_2 + c * 2^5 + 2^140 - t_P.
                // This is checked regardless of the value of d_1.
                let b2_c_prime_check = {
                    let two_pow_5 = pallas::Base::from(1 << 5);
                    let two_pow_140 =
                        Expression::Constant(pallas::Base::from_u128(1 << 70).square());
                    let t_p = Expression::Constant(pallas::Base::from_u128(T_P));
                    let b2_c_prime = meta.query_advice(config.advices[7], Rotation::next());
                    b_2 + c * two_pow_5 + two_pow_140 - t_p - b2_c_prime
                };

                // Check that the running sum output by the 140-bit little-
                // endian decomposition of b2_c_prime is zero.
                let z14_b2_c_prime = {
                    let z14_b2_c_prime = meta.query_advice(config.advices[8], Rotation::next());
                    d_1 * z14_b2_c_prime
                };

                iter::empty()
                    .chain(Some(("c0_canon_check", c0_canon_check)))
                    .chain(Some(("z13_c_check", z13_c_check)))
                    .chain(Some(("b2_c_prime_check", b2_c_prime_check)))
                    .chain(Some(("z14_b2_c_prime", z14_b2_c_prime)))
            };

            Constraints::with_selector(
                q_commit_ivk,
                iter::empty()
                    .chain(Some(("b1_bool_check", b1_bool_check)))
                    .chain(Some(("d1_bool_check", d1_bool_check)))
                    .chain(Some(("b_decomposition_check", b_decomposition_check)))
                    .chain(Some(("d_decomposition_check", d_decomposition_check)))
                    .chain(Some(("ak_decomposition_check", ak_decomposition_check)))
                    .chain(Some(("nk_decomposition_check", nk_decomposition_check)))
                    .chain(ak_canonicity_checks)
                    .chain(nk_canonicity_checks),
            )
        });

        config
    }

    pub(in crate::circuit) fn construct(config: CommitIvkConfig) -> Self {
        Self { config }
    }
}

pub(in crate::circuit) mod gadgets {
    use halo2_gadgets::utilities::{lookup_range_check::LookupRangeCheckConfig, RangeConstrained};
    use halo2_proofs::circuit::Chip;

    use super::*;

    /// `Commit^ivk` from [Section 5.4.8.4 Sinsemilla commitments].
    ///
    /// [Section 5.4.8.4 Sinsemilla commitments]: https://zips.z.cash/protocol/protocol.pdf#concretesinsemillacommit
    #[allow(non_snake_case)]
    #[allow(clippy::type_complexity)]
    pub(in crate::circuit) fn commit_ivk(
        sinsemilla_chip: SinsemillaChip<
            OrchardHashDomains,
            OrchardCommitDomains,
            OrchardFixedBases,
        >,
        ecc_chip: EccChip<OrchardFixedBases>,
        commit_ivk_chip: CommitIvkChip,
        mut layouter: impl Layouter<pallas::Base>,
        ak: AssignedCell<pallas::Base, pallas::Base>,
        nk: AssignedCell<pallas::Base, pallas::Base>,
        rivk: ScalarFixed<pallas::Affine, EccChip<OrchardFixedBases>>,
    ) -> Result<X<pallas::Affine, EccChip<OrchardFixedBases>>, Error> {
        let lookup_config = sinsemilla_chip.config().lookup_config();

        // We need to hash `ak || nk` where each of `ak`, `nk` is a field element (255 bits).
        //
        // a = bits 0..=249 of `ak`
        // b = b_0||b_1||b_2`
        //   = (bits 250..=253 of `ak`) || (bit 254 of  `ak`) || (bits 0..=4 of  `nk`)
        // c = bits 5..=244 of `nk`
        // d = d_0||d_1` = (bits 245..=253 of `nk`) || (bit 254 of `nk`)
        //
        // We start by witnessing all of the individual pieces, and range-constraining
        // the short pieces b_0, b_2, and d_0.
        //
        // https://p.z.cash/orchard-0.1:commit-ivk-bit-lengths?partial

        // `a` = bits 0..=249 of `ak`
        let a = MessagePiece::from_subpieces(
            sinsemilla_chip.clone(),
            layouter.namespace(|| "a"),
            [RangeConstrained::bitrange_of(ak.value(), 0..250)],
        )?;

        // `b = b_0||b_1||b_2`
        //    = (bits 250..=253 of `ak`) || (bit 254 of  `ak`) || (bits 0..=4 of  `nk`)
        let (b_0, b_1, b_2, b) = {
            // Constrain b_0 to be 4 bits.
            let b_0 = RangeConstrained::witness_short(
                &lookup_config,
                layouter.namespace(|| "b_0"),
                ak.value(),
                250..254,
            )?;
            // b_1 will be boolean-constrained in the custom gate.
            let b_1 = RangeConstrained::bitrange_of(ak.value(), 254..255);
            // Constrain b_2 to be 5 bits.
            let b_2 = RangeConstrained::witness_short(
                &lookup_config,
                layouter.namespace(|| "b_2"),
                nk.value(),
                0..5,
            )?;

            let b = MessagePiece::from_subpieces(
                sinsemilla_chip.clone(),
                layouter.namespace(|| "b = b_0 || b_1 || b_2"),
                [b_0.value(), b_1, b_2.value()],
            )?;

            (b_0, b_1, b_2, b)
        };

        // c = bits 5..=244 of `nk`
        let c = MessagePiece::from_subpieces(
            sinsemilla_chip.clone(),
            layouter.namespace(|| "c"),
            [RangeConstrained::bitrange_of(nk.value(), 5..245)],
        )?;

        // `d = d_0||d_1` = (bits 245..=253 of `nk`) || (bit 254 of `nk`)
        let (d_0, d_1, d) = {
            // Constrain d_0 to be 9 bits.
            let d_0 = RangeConstrained::witness_short(
                &lookup_config,
                layouter.namespace(|| "d_0"),
                nk.value(),
                245..254,
            )?;
            // d_1 will be boolean-constrained in the custom gate.
            let d_1 = RangeConstrained::bitrange_of(nk.value(), 254..255);

            let d = MessagePiece::from_subpieces(
                sinsemilla_chip.clone(),
                layouter.namespace(|| "d = d_0 || d_1"),
                [d_0.value(), d_1],
            )?;

            (d_0, d_1, d)
        };

        // ivk = Commit^ivk_rivk(I2LEBSP_255(ak) || I2LEBSP_255(nk))
        //
        // `ivk = ⊥` is handled internally to `CommitDomain::short_commit`: incomplete
        // addition constraints allows ⊥ to occur, and then during synthesis it detects
        // these edge cases and raises an error (aborting proof creation).
        //
        // https://p.z.cash/ZKS:action-addr-integrity?partial
        let (ivk, zs) = {
            let message = Message::from_pieces(
                sinsemilla_chip.clone(),
                vec![a.clone(), b.clone(), c.clone(), d.clone()],
            );
            let domain =
                CommitDomain::new(sinsemilla_chip, ecc_chip, &OrchardCommitDomains::CommitIvk);
            domain.short_commit(layouter.namespace(|| "Hash ak||nk"), message, rivk)?
        };

        // `CommitDomain::short_commit` returns the running sum for each `MessagePiece`.
        // Grab the outputs for pieces `a` and `c` that we will need for canonicity checks
        // on `ak` and `nk`.
        let z13_a = zs[0][13].clone();
        let z13_c = zs[2][13].clone();

        let (a_prime, z13_a_prime) = ak_canonicity(
            &lookup_config,
            layouter.namespace(|| "ak canonicity"),
            a.inner().cell_value(),
        )?;

        let (b2_c_prime, z14_b2_c_prime) = nk_canonicity(
            &lookup_config,
            layouter.namespace(|| "nk canonicity"),
            &b_2,
            c.inner().cell_value(),
        )?;

        let gate_cells = GateCells {
            a: a.inner().cell_value(),
            b: b.inner().cell_value(),
            c: c.inner().cell_value(),
            d: d.inner().cell_value(),
            ak,
            nk,
            b_0,
            b_1,
            b_2,
            d_0,
            d_1,
            z13_a,
            a_prime,
            z13_a_prime,
            z13_c,
            b2_c_prime,
            z14_b2_c_prime,
        };

        commit_ivk_chip.config.assign_gate(
            layouter.namespace(|| "Assign cells used in canonicity gate"),
            gate_cells,
        )?;

        Ok(ivk)
    }

    /// Witnesses and decomposes the `a'` value we need to check the canonicity of `ak`.
    ///
    /// [Specification](https://p.z.cash/orchard-0.1:commit-ivk-canonicity-ak?partial).
    #[allow(clippy::type_complexity)]
    fn ak_canonicity(
        lookup_config: &LookupRangeCheckConfig<pallas::Base, 10>,
        mut layouter: impl Layouter<pallas::Base>,
        a: AssignedCell<pallas::Base, pallas::Base>,
    ) -> Result<
        (
            AssignedCell<pallas::Base, pallas::Base>,
            AssignedCell<pallas::Base, pallas::Base>,
        ),
        Error,
    > {
        // `ak` = `a (250 bits) || b_0 (4 bits) || b_1 (1 bit)`
        // - b_1 = 1 => b_0 = 0
        // - b_1 = 1 => a < t_P
        //     - (0 ≤ a < 2^130) => z13_a of SinsemillaHash(a) == 0
        //     - 0 ≤ a + 2^130 - t_P < 2^130 (thirteen 10-bit lookups)

        // Decompose the low 130 bits of a_prime = a + 2^130 - t_P, and output
        // the running sum at the end of it. If a_prime < 2^130, the running sum
        // will be 0.
        let a_prime = {
            let two_pow_130 = Value::known(pallas::Base::from_u128(1u128 << 65).square());
            let t_p = Value::known(pallas::Base::from_u128(T_P));
            a.value() + two_pow_130 - t_p
        };
        let zs = lookup_config.witness_check(
            layouter.namespace(|| "Decompose low 130 bits of (a + 2^130 - t_P)"),
            a_prime,
            13,
            false,
        )?;
        let a_prime = zs[0].clone();
        assert_eq!(zs.len(), 14); // [z_0, z_1, ..., z13]

        Ok((a_prime, zs[13].clone()))
    }

    /// Witnesses and decomposes the `b2c'` value we need to check the canonicity of `nk`.
    ///
    /// [Specification](https://p.z.cash/orchard-0.1:commit-ivk-canonicity-nk?partial).
    #[allow(clippy::type_complexity)]
    fn nk_canonicity(
        lookup_config: &LookupRangeCheckConfig<pallas::Base, 10>,
        mut layouter: impl Layouter<pallas::Base>,
        b_2: &RangeConstrained<pallas::Base, AssignedCell<pallas::Base, pallas::Base>>,
        c: AssignedCell<pallas::Base, pallas::Base>,
    ) -> Result<
        (
            AssignedCell<pallas::Base, pallas::Base>,
            AssignedCell<pallas::Base, pallas::Base>,
        ),
        Error,
    > {
        // `nk` = `b_2 (5 bits) || c (240 bits) || d_0 (9 bits) || d_1 (1 bit)
        // - d_1 = 1 => d_0 = 0
        // - d_1 = 1 => b_2 + c * 2^5 < t_P
        //      - 0 ≤ b_2 + c * 2^5 < 2^140
        //          - b_2 was constrained to be 5 bits.
        //          - z_13 of SinsemillaHash(c) constrains bits 5..=134 to 130 bits
        //          - so b_2 + c * 2^5 is constrained to be 135 bits < 2^140.
        //      - 0 ≤ b_2 + c * 2^5 + 2^140 - t_P < 2^140 (14 ten-bit lookups)

        // Decompose the low 140 bits of b2_c_prime = b_2 + c * 2^5 + 2^140 - t_P, and output
        // the running sum at the end of it. If b2_c_prime < 2^140, the running sum will be 0.
        let b2_c_prime = {
            let two_pow_5 = Value::known(pallas::Base::from(1 << 5));
            let two_pow_140 = Value::known(pallas::Base::from_u128(1u128 << 70).square());
            let t_p = Value::known(pallas::Base::from_u128(T_P));
            b_2.inner().value() + c.value() * two_pow_5 + two_pow_140 - t_p
        };
        let zs = lookup_config.witness_check(
            layouter.namespace(|| "Decompose low 140 bits of (b_2 + c * 2^5 + 2^140 - t_P)"),
            b2_c_prime,
            14,
            false,
        )?;
        let b2_c_prime = zs[0].clone();
        assert_eq!(zs.len(), 15); // [z_0, z_1, ..., z14]

        Ok((b2_c_prime, zs[14].clone()))
    }
}

impl CommitIvkConfig {
    /// Assign cells for the [canonicity gate].
    ///
    /// [canonicity gate]: https://p.z.cash/orchard-0.1:commit-ivk-region-layout?partial
    /*
        The pieces are laid out in this configuration:

        |  A_0  |  A_1  |  A_2  |  A_3  |  A_4  |  A_5  |  A_6  |    A_7    |       A_8      | q_commit_ivk |
        -----------------------------------------------------------------------------------------------------
        |   ak  |   a   |   b   |  b_0  |  b_1  |  b_2  | z13_a |  a_prime  |   z13_a_prime  |       1      |
        |   nk  |   c   |   d   |  d_0  |  d_1  |       | z13_c | b2_c_prime| z14_b2_c_prime |       0      |

    */
    fn assign_gate(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        gate_cells: GateCells,
    ) -> Result<(), Error> {
        layouter.assign_region(
            || "Assign cells used in canonicity gate",
            |mut region| {
                // Enable selector on offset 0
                self.q_commit_ivk.enable(&mut region, 0)?;

                // Offset 0
                {
                    let offset = 0;
                    // Copy in `ak`
                    gate_cells
                        .ak
                        .copy_advice(|| "ak", &mut region, self.advices[0], offset)?;

                    // Copy in `a`
                    gate_cells
                        .a
                        .copy_advice(|| "a", &mut region, self.advices[1], offset)?;

                    // Copy in `b`
                    gate_cells
                        .b
                        .copy_advice(|| "b", &mut region, self.advices[2], offset)?;

                    // Copy in `b_0`
                    gate_cells.b_0.inner().copy_advice(
                        || "b_0",
                        &mut region,
                        self.advices[3],
                        offset,
                    )?;

                    // Witness `b_1`
                    region.assign_advice(
                        || "Witness b_1",
                        self.advices[4],
                        offset,
                        || *gate_cells.b_1.inner(),
                    )?;

                    // Copy in `b_2`
                    gate_cells.b_2.inner().copy_advice(
                        || "b_2",
                        &mut region,
                        self.advices[5],
                        offset,
                    )?;

                    // Copy in z13_a
                    gate_cells.z13_a.copy_advice(
                        || "z13_a",
                        &mut region,
                        self.advices[6],
                        offset,
                    )?;

                    // Copy in a_prime
                    gate_cells.a_prime.copy_advice(
                        || "a_prime",
                        &mut region,
                        self.advices[7],
                        offset,
                    )?;

                    // Copy in z13_a_prime
                    gate_cells.z13_a_prime.copy_advice(
                        || "z13_a_prime",
                        &mut region,
                        self.advices[8],
                        offset,
                    )?;
                }

                // Offset 1
                {
                    let offset = 1;

                    // Copy in `nk`
                    gate_cells
                        .nk
                        .copy_advice(|| "nk", &mut region, self.advices[0], offset)?;

                    // Copy in `c`
                    gate_cells
                        .c
                        .copy_advice(|| "c", &mut region, self.advices[1], offset)?;

                    // Copy in `d`
                    gate_cells
                        .d
                        .copy_advice(|| "d", &mut region, self.advices[2], offset)?;

                    // Copy in `d_0`
                    gate_cells.d_0.inner().copy_advice(
                        || "d_0",
                        &mut region,
                        self.advices[3],
                        offset,
                    )?;

                    // Witness `d_1`
                    region.assign_advice(
                        || "Witness d_1",
                        self.advices[4],
                        offset,
                        || *gate_cells.d_1.inner(),
                    )?;

                    // Copy in z13_c
                    gate_cells.z13_c.copy_advice(
                        || "z13_c",
                        &mut region,
                        self.advices[6],
                        offset,
                    )?;

                    // Copy in b2_c_prime
                    gate_cells.b2_c_prime.copy_advice(
                        || "b2_c_prime",
                        &mut region,
                        self.advices[7],
                        offset,
                    )?;

                    // Copy in z14_b2_c_prime
                    gate_cells.z14_b2_c_prime.copy_advice(
                        || "z14_b2_c_prime",
                        &mut region,
                        self.advices[8],
                        offset,
                    )?;
                }

                Ok(())
            },
        )
    }
}

// Cells used in the canonicity gate.
struct GateCells {
    a: AssignedCell<pallas::Base, pallas::Base>,
    b: AssignedCell<pallas::Base, pallas::Base>,
    c: AssignedCell<pallas::Base, pallas::Base>,
    d: AssignedCell<pallas::Base, pallas::Base>,
    ak: AssignedCell<pallas::Base, pallas::Base>,
    nk: AssignedCell<pallas::Base, pallas::Base>,
    b_0: RangeConstrained<pallas::Base, AssignedCell<pallas::Base, pallas::Base>>,
    b_1: RangeConstrained<pallas::Base, Value<pallas::Base>>,
    b_2: RangeConstrained<pallas::Base, AssignedCell<pallas::Base, pallas::Base>>,
    d_0: RangeConstrained<pallas::Base, AssignedCell<pallas::Base, pallas::Base>>,
    d_1: RangeConstrained<pallas::Base, Value<pallas::Base>>,
    z13_a: AssignedCell<pallas::Base, pallas::Base>,
    a_prime: AssignedCell<pallas::Base, pallas::Base>,
    z13_a_prime: AssignedCell<pallas::Base, pallas::Base>,
    z13_c: AssignedCell<pallas::Base, pallas::Base>,
    b2_c_prime: AssignedCell<pallas::Base, pallas::Base>,
    z14_b2_c_prime: AssignedCell<pallas::Base, pallas::Base>,
}

#[cfg(test)]
mod tests {
    use core::iter;

    use super::{gadgets, CommitIvkChip, CommitIvkConfig};
    use crate::constants::{
        fixed_bases::COMMIT_IVK_PERSONALIZATION, OrchardCommitDomains, OrchardFixedBases,
        OrchardHashDomains, L_ORCHARD_BASE, T_Q,
    };
    use group::ff::{Field, PrimeField, PrimeFieldBits};
    use halo2_gadgets::{
        ecc::{
            chip::{EccChip, EccConfig},
            ScalarFixed,
        },
        sinsemilla::{
            chip::{SinsemillaChip, SinsemillaConfig},
            primitives::CommitDomain,
        },
        utilities::{lookup_range_check::LookupRangeCheckConfig, UtilitiesInstructions},
    };
    use halo2_proofs::{
        circuit::{AssignedCell, Layouter, SimpleFloorPlanner, Value},
        dev::MockProver,
        plonk::{Circuit, ConstraintSystem, Error},
    };
    use pasta_curves::pallas;
    use rand::rngs::OsRng;

    #[test]
    fn commit_ivk() {
        #[derive(Default)]
        struct MyCircuit {
            ak: Value<pallas::Base>,
            nk: Value<pallas::Base>,
        }

        impl UtilitiesInstructions<pallas::Base> for MyCircuit {
            type Var = AssignedCell<pallas::Base, pallas::Base>;
        }

        impl Circuit<pallas::Base> for MyCircuit {
            type Config = (
                SinsemillaConfig<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>,
                CommitIvkConfig,
                EccConfig<OrchardFixedBases>,
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

                let constants = meta.fixed_column();
                meta.enable_constant(constants);

                for advice in advices.iter() {
                    meta.enable_equality(*advice);
                }

                let table_idx = meta.lookup_table_column();
                let lookup = (
                    table_idx,
                    meta.lookup_table_column(),
                    meta.lookup_table_column(),
                );
                let lagrange_coeffs = [
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                ];

                let range_check = LookupRangeCheckConfig::configure(meta, advices[9], table_idx);
                let sinsemilla_config = SinsemillaChip::<
                    OrchardHashDomains,
                    OrchardCommitDomains,
                    OrchardFixedBases,
                >::configure(
                    meta,
                    advices[..5].try_into().unwrap(),
                    advices[2],
                    lagrange_coeffs[0],
                    lookup,
                    range_check,
                );

                let commit_ivk_config = CommitIvkChip::configure(meta, advices);

                let ecc_config = EccChip::<OrchardFixedBases>::configure(
                    meta,
                    advices,
                    lagrange_coeffs,
                    range_check,
                );

                (sinsemilla_config, commit_ivk_config, ecc_config)
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<pallas::Base>,
            ) -> Result<(), Error> {
                let (sinsemilla_config, commit_ivk_config, ecc_config) = config;

                // Load the Sinsemilla generator lookup table used by the whole circuit.
                SinsemillaChip::<OrchardHashDomains, OrchardCommitDomains, OrchardFixedBases>::load(sinsemilla_config.clone(), &mut layouter)?;

                // Construct a Sinsemilla chip
                let sinsemilla_chip = SinsemillaChip::construct(sinsemilla_config);

                // Construct an ECC chip
                let ecc_chip = EccChip::construct(ecc_config);

                let commit_ivk_chip = CommitIvkChip::construct(commit_ivk_config.clone());

                // Witness ak
                let ak = self.load_private(
                    layouter.namespace(|| "load ak"),
                    commit_ivk_config.advices[0],
                    self.ak,
                )?;

                // Witness nk
                let nk = self.load_private(
                    layouter.namespace(|| "load nk"),
                    commit_ivk_config.advices[0],
                    self.nk,
                )?;

                // Use a random scalar for rivk
                let rivk = pallas::Scalar::random(OsRng);
                let rivk_gadget = ScalarFixed::new(
                    ecc_chip.clone(),
                    layouter.namespace(|| "rivk"),
                    Value::known(rivk),
                )?;

                let ivk = gadgets::commit_ivk(
                    sinsemilla_chip,
                    ecc_chip,
                    commit_ivk_chip,
                    layouter.namespace(|| "CommitIvk"),
                    ak,
                    nk,
                    rivk_gadget,
                )?;

                self.ak
                    .zip(self.nk)
                    .zip(ivk.inner().value())
                    .assert_if_known(|((ak, nk), ivk)| {
                        let expected_ivk = {
                            let domain = CommitDomain::new(COMMIT_IVK_PERSONALIZATION);
                            // Hash ak || nk
                            domain
                                .short_commit(
                                    iter::empty()
                                        .chain(
                                            ak.to_le_bits().iter().by_vals().take(L_ORCHARD_BASE),
                                        )
                                        .chain(
                                            nk.to_le_bits().iter().by_vals().take(L_ORCHARD_BASE),
                                        ),
                                    &rivk,
                                )
                                .unwrap()
                        };

                        &&expected_ivk == ivk
                    });

                Ok(())
            }
        }

        let two_pow_254 = pallas::Base::from_u128(1 << 127).square();
        // Test different values of `ak`, `nk`
        let circuits = [
            // `ak` = 0, `nk` = 0
            MyCircuit {
                ak: Value::known(pallas::Base::zero()),
                nk: Value::known(pallas::Base::zero()),
            },
            // `ak` = T_Q - 1, `nk` = T_Q - 1
            MyCircuit {
                ak: Value::known(pallas::Base::from_u128(T_Q - 1)),
                nk: Value::known(pallas::Base::from_u128(T_Q - 1)),
            },
            // `ak` = T_Q, `nk` = T_Q
            MyCircuit {
                ak: Value::known(pallas::Base::from_u128(T_Q)),
                nk: Value::known(pallas::Base::from_u128(T_Q)),
            },
            // `ak` = 2^127 - 1, `nk` = 2^127 - 1
            MyCircuit {
                ak: Value::known(pallas::Base::from_u128((1 << 127) - 1)),
                nk: Value::known(pallas::Base::from_u128((1 << 127) - 1)),
            },
            // `ak` = 2^127, `nk` = 2^127
            MyCircuit {
                ak: Value::known(pallas::Base::from_u128(1 << 127)),
                nk: Value::known(pallas::Base::from_u128(1 << 127)),
            },
            // `ak` = 2^254 - 1, `nk` = 2^254 - 1
            MyCircuit {
                ak: Value::known(two_pow_254 - pallas::Base::one()),
                nk: Value::known(two_pow_254 - pallas::Base::one()),
            },
            // `ak` = 2^254, `nk` = 2^254
            MyCircuit {
                ak: Value::known(two_pow_254),
                nk: Value::known(two_pow_254),
            },
        ];

        for circuit in circuits.iter() {
            let prover = MockProver::<pallas::Base>::run(11, circuit, vec![]).unwrap();
            assert_eq!(prover.verify(), Ok(()));
        }
    }
}
