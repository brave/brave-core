use super::super::{RoundWord, StateWord, STATE};
use super::{compression_util::*, CompressionConfig, State};
use halo2_proofs::{
    circuit::{Region, Value},
    pasta::pallas,
    plonk::Error,
};

impl CompressionConfig {
    #[allow(clippy::many_single_char_names)]
    pub fn initialize_iv(
        &self,
        region: &mut Region<'_, pallas::Base>,
        iv: [u32; STATE],
    ) -> Result<State, Error> {
        let a_7 = self.extras[3];

        // Decompose E into (6, 5, 14, 7)-bit chunks
        let e = self.decompose_e(region, RoundIdx::Init, Value::known(iv[4]))?;

        // Decompose F, G
        let f = self.decompose_f(region, InitialRound, Value::known(iv[5]))?;
        let g = self.decompose_g(region, InitialRound, Value::known(iv[6]))?;

        // Assign H
        let h_row = get_h_row(RoundIdx::Init);
        let h =
            self.assign_word_halves_dense(region, h_row, a_7, h_row + 1, a_7, Value::known(iv[7]))?;

        // Decompose A into (2, 11, 9, 10)-bit chunks
        let a = self.decompose_a(region, RoundIdx::Init, Value::known(iv[0]))?;

        // Decompose B, C
        let b = self.decompose_b(region, InitialRound, Value::known(iv[1]))?;
        let c = self.decompose_c(region, InitialRound, Value::known(iv[2]))?;

        // Assign D
        let d_row = get_d_row(RoundIdx::Init);
        let d =
            self.assign_word_halves_dense(region, d_row, a_7, d_row + 1, a_7, Value::known(iv[3]))?;

        Ok(State::new(
            StateWord::A(a),
            StateWord::B(b),
            StateWord::C(c),
            StateWord::D(d),
            StateWord::E(e),
            StateWord::F(f),
            StateWord::G(g),
            StateWord::H(h),
        ))
    }

    #[allow(clippy::many_single_char_names)]
    pub fn initialize_state(
        &self,
        region: &mut Region<'_, pallas::Base>,
        state: State,
    ) -> Result<State, Error> {
        let a_7 = self.extras[3];
        let (a, b, c, d, e, f, g, h) = match_state(state);

        // Decompose E into (6, 5, 14, 7)-bit chunks
        let e = e.dense_halves.value();
        let e = self.decompose_e(region, RoundIdx::Init, e)?;

        // Decompose F, G
        let f = f.dense_halves.value();
        let f = self.decompose_f(region, InitialRound, f)?;
        let g = g.dense_halves.value();
        let g = self.decompose_g(region, InitialRound, g)?;

        // Assign H
        let h = h.value();
        let h_row = get_h_row(RoundIdx::Init);
        let h = self.assign_word_halves_dense(region, h_row, a_7, h_row + 1, a_7, h)?;

        // Decompose A into (2, 11, 9, 10)-bit chunks
        let a = a.dense_halves.value();
        let a = self.decompose_a(region, RoundIdx::Init, a)?;

        // Decompose B, C
        let b = b.dense_halves.value();
        let b = self.decompose_b(region, InitialRound, b)?;
        let c = c.dense_halves.value();
        let c = self.decompose_c(region, InitialRound, c)?;

        // Assign D
        let d = d.value();
        let d_row = get_d_row(RoundIdx::Init);
        let d = self.assign_word_halves_dense(region, d_row, a_7, d_row + 1, a_7, d)?;

        Ok(State::new(
            StateWord::A(a),
            StateWord::B(b),
            StateWord::C(c),
            StateWord::D(d),
            StateWord::E(e),
            StateWord::F(f),
            StateWord::G(g),
            StateWord::H(h),
        ))
    }

    fn decompose_b(
        &self,
        region: &mut Region<'_, pallas::Base>,
        round_idx: InitialRound,
        b_val: Value<u32>,
    ) -> Result<RoundWord, Error> {
        let row = get_decompose_b_row(round_idx);

        let (dense_halves, spread_halves) = self.assign_word_halves(region, row, b_val)?;
        self.decompose_abcd(region, row, b_val)?;
        Ok(RoundWord::new(dense_halves, spread_halves))
    }

    fn decompose_c(
        &self,
        region: &mut Region<'_, pallas::Base>,
        round_idx: InitialRound,
        c_val: Value<u32>,
    ) -> Result<RoundWord, Error> {
        let row = get_decompose_c_row(round_idx);

        let (dense_halves, spread_halves) = self.assign_word_halves(region, row, c_val)?;
        self.decompose_abcd(region, row, c_val)?;
        Ok(RoundWord::new(dense_halves, spread_halves))
    }

    fn decompose_f(
        &self,
        region: &mut Region<'_, pallas::Base>,
        round_idx: InitialRound,
        f_val: Value<u32>,
    ) -> Result<RoundWord, Error> {
        let row = get_decompose_f_row(round_idx);

        let (dense_halves, spread_halves) = self.assign_word_halves(region, row, f_val)?;
        self.decompose_efgh(region, row, f_val)?;
        Ok(RoundWord::new(dense_halves, spread_halves))
    }

    fn decompose_g(
        &self,
        region: &mut Region<'_, pallas::Base>,
        round_idx: InitialRound,
        g_val: Value<u32>,
    ) -> Result<RoundWord, Error> {
        let row = get_decompose_g_row(round_idx);

        let (dense_halves, spread_halves) = self.assign_word_halves(region, row, g_val)?;
        self.decompose_efgh(region, row, g_val)?;
        Ok(RoundWord::new(dense_halves, spread_halves))
    }
}
