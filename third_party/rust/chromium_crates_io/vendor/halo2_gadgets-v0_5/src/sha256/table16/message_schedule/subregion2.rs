use super::super::{util::*, AssignedBits, Bits, SpreadVar, SpreadWord, Table16Assignment};
use super::{schedule_util::*, MessageScheduleConfig, MessageWord};
use halo2_proofs::{
    circuit::{Region, Value},
    pasta::pallas,
    plonk::Error,
};
use std::convert::TryInto;

/// A word in subregion 2
/// (3, 4, 3, 7, 1, 1, 13)-bit chunks
#[derive(Clone, Debug)]
pub struct Subregion2Word {
    index: usize,
    a: AssignedBits<3>,
    b: AssignedBits<4>,
    c: AssignedBits<3>,
    _d: AssignedBits<7>,
    e: AssignedBits<1>,
    f: AssignedBits<1>,
    _g: AssignedBits<13>,
    spread_d: AssignedBits<14>,
    spread_g: AssignedBits<26>,
}

impl Subregion2Word {
    fn spread_a(&self) -> Value<[bool; 6]> {
        self.a.value().map(|v| v.spread())
    }

    fn spread_b(&self) -> Value<[bool; 8]> {
        self.b.value().map(|v| v.spread())
    }

    fn spread_c(&self) -> Value<[bool; 6]> {
        self.c.value().map(|v| v.spread())
    }

    fn spread_d(&self) -> Value<[bool; 14]> {
        self.spread_d.value().map(|v| v.0)
    }

    fn spread_e(&self) -> Value<[bool; 2]> {
        self.e.value().map(|v| v.spread())
    }

    fn spread_f(&self) -> Value<[bool; 2]> {
        self.f.value().map(|v| v.spread())
    }

    fn spread_g(&self) -> Value<[bool; 26]> {
        self.spread_g.value().map(|v| v.0)
    }

    fn xor_sigma_0(&self) -> Value<[bool; 64]> {
        self.spread_a()
            .zip(self.spread_b())
            .zip(self.spread_c())
            .zip(self.spread_d())
            .zip(self.spread_e())
            .zip(self.spread_f())
            .zip(self.spread_g())
            .map(|((((((a, b), c), d), e), f), g)| {
                let xor_0 = b
                    .iter()
                    .chain(c.iter())
                    .chain(d.iter())
                    .chain(e.iter())
                    .chain(f.iter())
                    .chain(g.iter())
                    .chain(std::iter::repeat(&false).take(6))
                    .copied()
                    .collect::<Vec<_>>();

                let xor_1 = c
                    .iter()
                    .chain(d.iter())
                    .chain(e.iter())
                    .chain(f.iter())
                    .chain(g.iter())
                    .chain(a.iter())
                    .chain(b.iter())
                    .copied()
                    .collect::<Vec<_>>();

                let xor_2 = f
                    .iter()
                    .chain(g.iter())
                    .chain(a.iter())
                    .chain(b.iter())
                    .chain(c.iter())
                    .chain(d.iter())
                    .chain(e.iter())
                    .copied()
                    .collect::<Vec<_>>();

                let xor_0 = lebs2ip::<64>(&xor_0.try_into().unwrap());
                let xor_1 = lebs2ip::<64>(&xor_1.try_into().unwrap());
                let xor_2 = lebs2ip::<64>(&xor_2.try_into().unwrap());

                i2lebsp(xor_0 + xor_1 + xor_2)
            })
    }

    fn xor_sigma_1(&self) -> Value<[bool; 64]> {
        self.spread_a()
            .zip(self.spread_b())
            .zip(self.spread_c())
            .zip(self.spread_d())
            .zip(self.spread_e())
            .zip(self.spread_f())
            .zip(self.spread_g())
            .map(|((((((a, b), c), d), e), f), g)| {
                let xor_0 = d
                    .iter()
                    .chain(e.iter())
                    .chain(f.iter())
                    .chain(g.iter())
                    .chain(std::iter::repeat(&false).take(20))
                    .copied()
                    .collect::<Vec<_>>();

                let xor_1 = e
                    .iter()
                    .chain(f.iter())
                    .chain(g.iter())
                    .chain(a.iter())
                    .chain(b.iter())
                    .chain(c.iter())
                    .chain(d.iter())
                    .copied()
                    .collect::<Vec<_>>();

                let xor_2 = g
                    .iter()
                    .chain(a.iter())
                    .chain(b.iter())
                    .chain(c.iter())
                    .chain(d.iter())
                    .chain(e.iter())
                    .chain(f.iter())
                    .copied()
                    .collect::<Vec<_>>();

                let xor_0 = lebs2ip::<64>(&xor_0.try_into().unwrap());
                let xor_1 = lebs2ip::<64>(&xor_1.try_into().unwrap());
                let xor_2 = lebs2ip::<64>(&xor_2.try_into().unwrap());

                i2lebsp(xor_0 + xor_1 + xor_2)
            })
    }
}

impl MessageScheduleConfig {
    // W_[14..49]
    pub fn assign_subregion2(
        &self,
        region: &mut Region<'_, pallas::Base>,
        lower_sigma_0_output: Vec<(AssignedBits<16>, AssignedBits<16>)>,
        w: &mut Vec<MessageWord>,
        w_halves: &mut Vec<(AssignedBits<16>, AssignedBits<16>)>,
    ) -> Result<Vec<(AssignedBits<16>, AssignedBits<16>)>, Error> {
        let a_5 = self.message_schedule;
        let a_6 = self.extras[2];
        let a_7 = self.extras[3];
        let a_8 = self.extras[4];
        let a_9 = self.extras[5];

        let mut lower_sigma_0_v2_results =
            Vec::<(AssignedBits<16>, AssignedBits<16>)>::with_capacity(SUBREGION_2_LEN);
        let mut lower_sigma_1_v2_results =
            Vec::<(AssignedBits<16>, AssignedBits<16>)>::with_capacity(SUBREGION_2_LEN);

        // Closure to compose new word
        // W_i = sigma_1(W_{i - 2}) + W_{i - 7} + sigma_0(W_{i - 15}) + W_{i - 16}
        // e.g. W_16 = sigma_1(W_14) + W_9 + sigma_0(W_1) + W_0

        // sigma_0(W_[1..14]) will be used to get the new W_[16..29]
        // sigma_0_v2(W_[14..36]) will be used to get the new W_[29..51]
        // sigma_1_v2(W_[14..49]) will be used to get the W_[16..51]
        // The lowest-index words involved will be W_[0..13]
        let mut new_word = |idx: usize,
                            sigma_0_output: &(AssignedBits<16>, AssignedBits<16>)|
         -> Result<Vec<(AssignedBits<16>, AssignedBits<16>)>, Error> {
            // Decompose word into (3, 4, 3, 7, 1, 1, 13)-bit chunks
            let word = self.decompose_word(region, w[idx].value(), idx)?;

            // sigma_0 v2 and sigma_1 v2 on word
            lower_sigma_0_v2_results.push(self.lower_sigma_0_v2(region, word.clone())?);
            lower_sigma_1_v2_results.push(self.lower_sigma_1_v2(region, word)?);

            let new_word_idx = idx + 2;

            // Copy sigma_0(W_{i - 15}) output from Subregion 1
            sigma_0_output.0.copy_advice(
                || format!("sigma_0(W_{})_lo", new_word_idx - 15),
                region,
                a_6,
                get_word_row(new_word_idx - 16),
            )?;
            sigma_0_output.1.copy_advice(
                || format!("sigma_0(W_{})_hi", new_word_idx - 15),
                region,
                a_6,
                get_word_row(new_word_idx - 16) + 1,
            )?;

            // Copy sigma_1(W_{i - 2})
            lower_sigma_1_v2_results[new_word_idx - 16].0.copy_advice(
                || format!("sigma_1(W_{})_lo", new_word_idx - 2),
                region,
                a_7,
                get_word_row(new_word_idx - 16),
            )?;
            lower_sigma_1_v2_results[new_word_idx - 16].1.copy_advice(
                || format!("sigma_1(W_{})_hi", new_word_idx - 2),
                region,
                a_7,
                get_word_row(new_word_idx - 16) + 1,
            )?;

            // Copy W_{i - 7}
            w_halves[new_word_idx - 7].0.copy_advice(
                || format!("W_{}_lo", new_word_idx - 7),
                region,
                a_8,
                get_word_row(new_word_idx - 16),
            )?;
            w_halves[new_word_idx - 7].1.copy_advice(
                || format!("W_{}_hi", new_word_idx - 7),
                region,
                a_8,
                get_word_row(new_word_idx - 16) + 1,
            )?;

            // Calculate W_i, carry_i
            let (word, carry) = sum_with_carry(vec![
                (
                    lower_sigma_1_v2_results[new_word_idx - 16].0.value_u16(),
                    lower_sigma_1_v2_results[new_word_idx - 16].1.value_u16(),
                ),
                (
                    w_halves[new_word_idx - 7].0.value_u16(),
                    w_halves[new_word_idx - 7].1.value_u16(),
                ),
                (sigma_0_output.0.value_u16(), sigma_0_output.1.value_u16()),
                (
                    w_halves[new_word_idx - 16].0.value_u16(),
                    w_halves[new_word_idx - 16].1.value_u16(),
                ),
            ]);

            // Assign W_i, carry_i
            region.assign_advice(
                || format!("W_{}", new_word_idx),
                a_5,
                get_word_row(new_word_idx - 16) + 1,
                || word.map(|word| pallas::Base::from(word as u64)),
            )?;
            region.assign_advice(
                || format!("carry_{}", new_word_idx),
                a_9,
                get_word_row(new_word_idx - 16) + 1,
                || carry.map(pallas::Base::from),
            )?;
            let (word, halves) = self.assign_word_and_halves(region, word, new_word_idx)?;
            w.push(MessageWord(word));
            w_halves.push(halves);

            Ok(lower_sigma_0_v2_results.clone())
        };

        let mut tmp_lower_sigma_0_v2_results: Vec<(AssignedBits<16>, AssignedBits<16>)> =
            Vec::with_capacity(SUBREGION_2_LEN);

        // Use up all the output from Subregion 1 lower_sigma_0
        for i in 14..27 {
            tmp_lower_sigma_0_v2_results = new_word(i, &lower_sigma_0_output[i - 14])?;
        }

        for i in 27..49 {
            tmp_lower_sigma_0_v2_results =
                new_word(i, &tmp_lower_sigma_0_v2_results[i + 2 - 15 - 14])?;
        }

        // Return lower_sigma_0_v2 output for W_[36..49]
        Ok(lower_sigma_0_v2_results.split_off(36 - 14))
    }

    /// Pieces of length [3, 4, 3, 7, 1, 1, 13]
    fn decompose_word(
        &self,
        region: &mut Region<'_, pallas::Base>,
        word: Value<&Bits<32>>,
        index: usize,
    ) -> Result<Subregion2Word, Error> {
        let row = get_word_row(index);

        let pieces = word.map(|word| {
            vec![
                word[0..3].to_vec(),
                word[3..7].to_vec(),
                word[7..10].to_vec(),
                word[10..17].to_vec(),
                vec![word[17]],
                vec![word[18]],
                word[19..32].to_vec(),
            ]
        });
        let pieces = pieces.transpose_vec(7);

        // Rename these here for ease of matching the gates to the specification.
        let a_3 = self.extras[0];
        let a_4 = self.extras[1];

        // Assign `a` (3-bit piece)
        let a = AssignedBits::<3>::assign_bits(region, || "a", a_3, row - 1, pieces[0].clone())?;

        // Assign `b` (4-bit piece) lookup
        let spread_b: Value<SpreadWord<4, 8>> = pieces[1].clone().map(SpreadWord::try_new);
        let spread_b = SpreadVar::with_lookup(region, &self.lookup, row + 1, spread_b)?;

        // Assign `c` (3-bit piece)
        let c = AssignedBits::<3>::assign_bits(region, || "c", a_4, row - 1, pieces[2].clone())?;

        // Assign `d` (7-bit piece) lookup
        let spread_d: Value<SpreadWord<7, 14>> = pieces[3].clone().map(SpreadWord::try_new);
        let spread_d = SpreadVar::with_lookup(region, &self.lookup, row, spread_d)?;

        // Assign `e` (1-bit piece)
        let e = AssignedBits::<1>::assign_bits(region, || "e", a_3, row + 1, pieces[4].clone())?;

        // Assign `f` (1-bit piece)
        let f = AssignedBits::<1>::assign_bits(region, || "f", a_4, row + 1, pieces[5].clone())?;

        // Assign `g` (13-bit piece) lookup
        let spread_g = pieces[6].clone().map(SpreadWord::try_new);
        let spread_g = SpreadVar::with_lookup(region, &self.lookup, row - 1, spread_g)?;

        Ok(Subregion2Word {
            index,
            a,
            b: spread_b.dense,
            c,
            _d: spread_d.dense,
            e,
            f,
            _g: spread_g.dense,
            spread_d: spread_d.spread,
            spread_g: spread_g.spread,
        })
    }

    /// A word in subregion 2
    /// (3, 4, 3, 7, 1, 1, 13)-bit chunks
    #[allow(clippy::type_complexity)]
    fn assign_lower_sigma_v2_pieces(
        &self,
        region: &mut Region<'_, pallas::Base>,
        row: usize,
        word: &Subregion2Word,
    ) -> Result<(), Error> {
        let a_3 = self.extras[0];
        let a_4 = self.extras[1];
        let a_5 = self.message_schedule;
        let a_6 = self.extras[2];
        let a_7 = self.extras[3];

        // Assign `a` and copy constraint
        word.a.copy_advice(|| "a", region, a_3, row + 1)?;

        // Witness `spread_a`
        AssignedBits::<6>::assign_bits(region, || "spread_a", a_4, row + 1, word.spread_a())?;

        // Split `b` (4-bit chunk) into `b_hi` and `b_lo`
        // Assign `b_lo`, `spread_b_lo`

        let b_lo: Value<[bool; 2]> = word.b.value().map(|b| b.0[..2].try_into().unwrap());
        let spread_b_lo = b_lo.map(spread_bits);
        {
            AssignedBits::<2>::assign_bits(region, || "b_lo", a_3, row - 1, b_lo)?;

            AssignedBits::<4>::assign_bits(region, || "spread_b_lo", a_4, row - 1, spread_b_lo)?;
        };

        // Split `b` (2-bit chunk) into `b_hi` and `b_lo`
        // Assign `b_hi`, `spread_b_hi`
        let b_hi: Value<[bool; 2]> = word.b.value().map(|b| b.0[2..].try_into().unwrap());
        let spread_b_hi = b_hi.map(spread_bits);
        {
            AssignedBits::<2>::assign_bits(region, || "b_hi", a_5, row - 1, b_hi)?;

            AssignedBits::<4>::assign_bits(region, || "spread_b_hi", a_6, row - 1, spread_b_hi)?;
        };

        // Assign `b` and copy constraint
        word.b.copy_advice(|| "b", region, a_6, row)?;

        // Assign `c` and copy constraint
        word.c.copy_advice(|| "c", region, a_5, row + 1)?;

        // Witness `spread_c`
        AssignedBits::<6>::assign_bits(region, || "spread_c", a_6, row + 1, word.spread_c())?;

        // Assign `spread_d` and copy constraint
        word.spread_d.copy_advice(|| "spread_d", region, a_4, row)?;

        // Assign `e` and copy constraint
        word.e.copy_advice(|| "e", region, a_7, row)?;

        // Assign `f` and copy constraint
        word.f.copy_advice(|| "f", region, a_7, row + 1)?;

        // Assign `spread_g` and copy constraint
        word.spread_g.copy_advice(|| "spread_g", region, a_5, row)?;

        Ok(())
    }

    fn lower_sigma_0_v2(
        &self,
        region: &mut Region<'_, pallas::Base>,
        word: Subregion2Word,
    ) -> Result<(AssignedBits<16>, AssignedBits<16>), Error> {
        let a_3 = self.extras[0];
        let row = get_word_row(word.index) + 3;

        // Assign lower sigma_v2 pieces
        self.assign_lower_sigma_v2_pieces(region, row, &word)?;

        // Calculate R_0^{even}, R_0^{odd}, R_1^{even}, R_1^{odd}
        let r = word.xor_sigma_0();
        let r_0: Value<[bool; 32]> = r.map(|r| r[..32].try_into().unwrap());
        let r_0_even = r_0.map(even_bits);
        let r_0_odd = r_0.map(odd_bits);

        let r_1: Value<[bool; 32]> = r.map(|r| r[32..].try_into().unwrap());
        let r_1_even = r_1.map(even_bits);
        let r_1_odd = r_1.map(odd_bits);

        self.assign_sigma_outputs(
            region,
            &self.lookup,
            a_3,
            row,
            r_0_even,
            r_0_odd,
            r_1_even,
            r_1_odd,
        )
    }

    fn lower_sigma_1_v2(
        &self,
        region: &mut Region<'_, pallas::Base>,
        word: Subregion2Word,
    ) -> Result<(AssignedBits<16>, AssignedBits<16>), Error> {
        let a_3 = self.extras[0];
        let row = get_word_row(word.index) + SIGMA_0_V2_ROWS + 3;

        // Assign lower sigma_v2 pieces
        self.assign_lower_sigma_v2_pieces(region, row, &word)?;

        // (3, 4, 3, 7, 1, 1, 13)
        // Calculate R_0^{even}, R_0^{odd}, R_1^{even}, R_1^{odd}
        // Calculate R_0^{even}, R_0^{odd}, R_1^{even}, R_1^{odd}
        let r = word.xor_sigma_1();
        let r_0: Value<[bool; 32]> = r.map(|r| r[..32].try_into().unwrap());
        let r_0_even = r_0.map(even_bits);
        let r_0_odd = r_0.map(odd_bits);

        let r_1: Value<[bool; 32]> = r.map(|r| r[32..].try_into().unwrap());
        let r_1_even = r_1.map(even_bits);
        let r_1_odd = r_1.map(odd_bits);

        self.assign_sigma_outputs(
            region,
            &self.lookup,
            a_3,
            row,
            r_0_even,
            r_0_odd,
            r_1_even,
            r_1_odd,
        )
    }
}
