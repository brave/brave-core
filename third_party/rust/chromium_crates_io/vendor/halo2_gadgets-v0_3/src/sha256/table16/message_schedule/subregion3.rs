use super::super::{util::*, AssignedBits, Bits, SpreadVar, SpreadWord, Table16Assignment};
use super::{schedule_util::*, MessageScheduleConfig, MessageWord};
use halo2_proofs::{
    circuit::{Region, Value},
    pasta::pallas,
    plonk::Error,
};
use std::convert::TryInto;

// A word in subregion 3
// (10, 7, 2, 13)-bit chunks
pub struct Subregion3Word {
    index: usize,
    #[allow(dead_code)]
    a: AssignedBits<10>,
    b: AssignedBits<7>,
    c: AssignedBits<2>,
    #[allow(dead_code)]
    d: AssignedBits<13>,
    spread_a: AssignedBits<20>,
    spread_d: AssignedBits<26>,
}

impl Subregion3Word {
    fn spread_a(&self) -> Value<[bool; 20]> {
        self.spread_a.value().map(|v| v.0)
    }

    fn spread_b(&self) -> Value<[bool; 14]> {
        self.b.value().map(|v| v.spread())
    }

    fn spread_c(&self) -> Value<[bool; 4]> {
        self.c.value().map(|v| v.spread())
    }

    fn spread_d(&self) -> Value<[bool; 26]> {
        self.spread_d.value().map(|v| v.0)
    }

    fn xor_lower_sigma_1(&self) -> Value<[bool; 64]> {
        self.spread_a()
            .zip(self.spread_b())
            .zip(self.spread_c())
            .zip(self.spread_d())
            .map(|(((a, b), c), d)| {
                let xor_0 = b
                    .iter()
                    .chain(c.iter())
                    .chain(d.iter())
                    .chain(std::iter::repeat(&false).take(20))
                    .copied()
                    .collect::<Vec<_>>();

                let xor_1 = c
                    .iter()
                    .chain(d.iter())
                    .chain(a.iter())
                    .chain(b.iter())
                    .copied()
                    .collect::<Vec<_>>();
                let xor_2 = d
                    .iter()
                    .chain(a.iter())
                    .chain(b.iter())
                    .chain(c.iter())
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
    // W_[49..62]
    pub fn assign_subregion3(
        &self,
        region: &mut Region<'_, pallas::Base>,
        lower_sigma_0_v2_output: Vec<(AssignedBits<16>, AssignedBits<16>)>,
        w: &mut Vec<MessageWord>,
        w_halves: &mut Vec<(AssignedBits<16>, AssignedBits<16>)>,
    ) -> Result<(), Error> {
        let a_5 = self.message_schedule;
        let a_6 = self.extras[2];
        let a_7 = self.extras[3];
        let a_8 = self.extras[4];
        let a_9 = self.extras[5];

        // Closure to compose new word
        // W_i = sigma_1(W_{i - 2}) + W_{i - 7} + sigma_0(W_{i - 15}) + W_{i - 16}
        // e.g. W_51 = sigma_1(W_49) + W_44 + sigma_0(W_36) + W_35

        // sigma_0_v2(W_[36..49]) will be used to get the new W_[51..64]
        // sigma_1(W_[49..62]) will also be used to get the W_[51..64]
        // The lowest-index words involved will be W_[35..58]
        let mut new_word = |idx: usize| -> Result<(), Error> {
            // Decompose word into (10, 7, 2, 13)-bit chunks
            let subregion3_word = self.decompose_subregion3_word(region, w[idx].value(), idx)?;

            // sigma_1 on subregion3_word
            let (r_0_even, r_1_even) = self.lower_sigma_1(region, subregion3_word)?;

            let new_word_idx = idx + 2;

            // Copy sigma_0_v2(W_{i - 15}) output from Subregion 2
            lower_sigma_0_v2_output[idx - 49].0.copy_advice(
                || format!("sigma_0(W_{})_lo", new_word_idx - 15),
                region,
                a_6,
                get_word_row(new_word_idx - 16),
            )?;
            lower_sigma_0_v2_output[idx - 49].1.copy_advice(
                || format!("sigma_0(W_{})_hi", new_word_idx - 15),
                region,
                a_6,
                get_word_row(new_word_idx - 16) + 1,
            )?;

            // Copy sigma_1(W_{i - 2})
            r_0_even.copy_advice(
                || format!("sigma_1(W_{})_lo", new_word_idx - 2),
                region,
                a_7,
                get_word_row(new_word_idx - 16),
            )?;
            r_1_even.copy_advice(
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
                (r_0_even.value_u16(), r_1_even.value_u16()),
                (
                    w_halves[new_word_idx - 7].0.value_u16(),
                    w_halves[new_word_idx - 7].1.value_u16(),
                ),
                (
                    lower_sigma_0_v2_output[idx - 49].0.value_u16(),
                    lower_sigma_0_v2_output[idx - 49].1.value_u16(),
                ),
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

            Ok(())
        };

        for i in 49..62 {
            new_word(i)?;
        }

        Ok(())
    }

    /// Pieces of length [10, 7, 2, 13]
    fn decompose_subregion3_word(
        &self,
        region: &mut Region<'_, pallas::Base>,
        word: Value<&Bits<32>>,
        index: usize,
    ) -> Result<Subregion3Word, Error> {
        let row = get_word_row(index);

        // Rename these here for ease of matching the gates to the specification.
        let a_3 = self.extras[0];
        let a_4 = self.extras[1];

        let pieces = word.map(|word| {
            vec![
                word[0..10].to_vec(),
                word[10..17].to_vec(),
                word[17..19].to_vec(),
                word[19..32].to_vec(),
            ]
        });
        let pieces = pieces.transpose_vec(4);

        // Assign `a` (10-bit piece)
        let spread_a = pieces[0].clone().map(SpreadWord::try_new);
        let spread_a = SpreadVar::with_lookup(region, &self.lookup, row + 1, spread_a)?;

        // Assign `b` (7-bit piece)
        let b = AssignedBits::<7>::assign_bits(region, || "b", a_4, row + 1, pieces[1].clone())?;

        // Assign `c` (2-bit piece)
        let c = AssignedBits::<2>::assign_bits(region, || "c", a_3, row + 1, pieces[2].clone())?;

        // Assign `d` (13-bit piece) lookup
        let spread_d = pieces[3].clone().map(SpreadWord::try_new);
        let spread_d = SpreadVar::with_lookup(region, &self.lookup, row, spread_d)?;

        Ok(Subregion3Word {
            index,
            a: spread_a.dense,
            b,
            c,
            d: spread_d.dense,
            spread_a: spread_a.spread,
            spread_d: spread_d.spread,
        })
    }

    fn lower_sigma_1(
        &self,
        region: &mut Region<'_, pallas::Base>,
        word: Subregion3Word,
    ) -> Result<(AssignedBits<16>, AssignedBits<16>), Error> {
        let a_3 = self.extras[0];
        let a_4 = self.extras[1];
        let a_5 = self.message_schedule;
        let a_6 = self.extras[2];

        let row = get_word_row(word.index) + 3;

        // Assign `spread_a` and copy constraint
        word.spread_a.copy_advice(|| "spread_a", region, a_4, row)?;

        // Split `b` (7-bit chunk) into (2, 2, 3)-bit `b_lo`, `b_mid` and `b_hi`.
        // Assign `b_lo`, `spread_b_lo`, `b_mid`, `spread_b_mid`, `b_hi`, `spread_b_hi`.

        // b_lo (2-bit chunk)
        {
            let b_lo: Value<[bool; 2]> = word.b.value().map(|v| v[0..2].try_into().unwrap());
            let b_lo = b_lo.map(SpreadWord::<2, 4>::new);
            SpreadVar::without_lookup(region, a_3, row - 1, a_4, row - 1, b_lo)?;
        }

        // b_mid (2-bit chunk)
        {
            let b_mid: Value<[bool; 2]> = word.b.value().map(|v| v[2..4].try_into().unwrap());
            let b_mid = b_mid.map(SpreadWord::<2, 4>::new);
            SpreadVar::without_lookup(region, a_5, row - 1, a_6, row - 1, b_mid)?;
        }

        // b_hi (3-bit chunk)
        {
            let b_hi: Value<[bool; 3]> = word.b.value().map(|v| v[4..7].try_into().unwrap());
            let b_hi = b_hi.map(SpreadWord::<3, 6>::new);
            SpreadVar::without_lookup(region, a_5, row + 1, a_6, row + 1, b_hi)?;
        }

        // Assign `b` and copy constraint
        word.b.copy_advice(|| "b", region, a_6, row)?;

        // Assign `c` and copy constraint
        word.c.copy_advice(|| "c", region, a_3, row + 1)?;

        // Witness `spread_c`
        {
            let spread_c = word.c.value().map(spread_bits);
            AssignedBits::<4>::assign_bits(region, || "spread_c", a_4, row + 1, spread_c)?;
        }

        // Assign `spread_d` and copy constraint
        word.spread_d.copy_advice(|| "spread_d", region, a_5, row)?;

        // (10, 7, 2, 13)
        // Calculate R_0^{even}, R_0^{odd}, R_1^{even}, R_1^{odd}
        let r = word.xor_lower_sigma_1();
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
