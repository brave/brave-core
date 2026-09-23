use super::super::{super::DIGEST_SIZE, BlockWord, RoundWordDense};
use super::{compression_util::*, CompressionConfig, State};
use halo2_proofs::{
    circuit::{Region, Value},
    pasta::pallas,
    plonk::{Advice, Column, Error},
};

impl CompressionConfig {
    #[allow(clippy::many_single_char_names)]
    pub fn assign_digest(
        &self,
        region: &mut Region<'_, pallas::Base>,
        state: State,
    ) -> Result<[BlockWord; DIGEST_SIZE], Error> {
        let a_3 = self.extras[0];
        let a_4 = self.extras[1];
        let a_5 = self.message_schedule;
        let a_6 = self.extras[2];
        let a_7 = self.extras[3];
        let a_8 = self.extras[4];

        let (a, b, c, d, e, f, g, h) = match_state(state);

        let abcd_row = 0;
        self.s_digest.enable(region, abcd_row)?;
        let efgh_row = abcd_row + 2;
        self.s_digest.enable(region, efgh_row)?;

        // Assign digest for A, B, C, D
        a.dense_halves
            .0
            .copy_advice(|| "a_lo", region, a_3, abcd_row)?;
        a.dense_halves
            .1
            .copy_advice(|| "a_hi", region, a_4, abcd_row)?;
        let a = a.dense_halves.value();
        region.assign_advice(
            || "a",
            a_5,
            abcd_row,
            || a.map(|a| pallas::Base::from(a as u64)),
        )?;

        let b = self.assign_digest_word(region, abcd_row, a_6, a_7, a_8, b.dense_halves)?;
        let c = self.assign_digest_word(region, abcd_row + 1, a_3, a_4, a_5, c.dense_halves)?;
        let d = self.assign_digest_word(region, abcd_row + 1, a_6, a_7, a_8, d)?;

        // Assign digest for E, F, G, H
        e.dense_halves
            .0
            .copy_advice(|| "e_lo", region, a_3, efgh_row)?;
        e.dense_halves
            .1
            .copy_advice(|| "e_hi", region, a_4, efgh_row)?;
        let e = e.dense_halves.value();
        region.assign_advice(
            || "e",
            a_5,
            efgh_row,
            || e.map(|e| pallas::Base::from(e as u64)),
        )?;

        let f = self.assign_digest_word(region, efgh_row, a_6, a_7, a_8, f.dense_halves)?;
        let g = self.assign_digest_word(region, efgh_row + 1, a_3, a_4, a_5, g.dense_halves)?;
        let h = self.assign_digest_word(region, efgh_row + 1, a_6, a_7, a_8, h)?;

        Ok([
            BlockWord(a),
            BlockWord(b),
            BlockWord(c),
            BlockWord(d),
            BlockWord(e),
            BlockWord(f),
            BlockWord(g),
            BlockWord(h),
        ])
    }

    fn assign_digest_word(
        &self,
        region: &mut Region<'_, pallas::Base>,
        row: usize,
        lo_col: Column<Advice>,
        hi_col: Column<Advice>,
        word_col: Column<Advice>,
        dense_halves: RoundWordDense,
    ) -> Result<Value<u32>, Error> {
        dense_halves.0.copy_advice(|| "lo", region, lo_col, row)?;
        dense_halves.1.copy_advice(|| "hi", region, hi_col, row)?;

        let val = dense_halves.value();
        region.assign_advice(
            || "word",
            word_col,
            row,
            || val.map(|val| pallas::Base::from(val as u64)),
        )?;

        Ok(val)
    }
}
