use super::super::Gate;

use group::ff::{Field, PrimeField};
use halo2_proofs::plonk::Expression;
use std::marker::PhantomData;

pub struct ScheduleGate<F: Field>(PhantomData<F>);

impl<F: PrimeField> ScheduleGate<F> {
    /// s_word for W_16 to W_63
    #[allow(clippy::too_many_arguments)]
    pub fn s_word(
        s_word: Expression<F>,
        sigma_0_lo: Expression<F>,
        sigma_0_hi: Expression<F>,
        sigma_1_lo: Expression<F>,
        sigma_1_hi: Expression<F>,
        w_minus_9_lo: Expression<F>,
        w_minus_9_hi: Expression<F>,
        w_minus_16_lo: Expression<F>,
        w_minus_16_hi: Expression<F>,
        word: Expression<F>,
        carry: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let lo = sigma_0_lo + sigma_1_lo + w_minus_9_lo + w_minus_16_lo;
        let hi = sigma_0_hi + sigma_1_hi + w_minus_9_hi + w_minus_16_hi;

        let word_check = lo
            + hi * F::from(1 << 16)
            + (carry.clone() * F::from(1 << 32) * (-F::ONE))
            + (word * (-F::ONE));
        let carry_check = Gate::range_check(carry, 0, 3);

        [("word_check", word_check), ("carry_check", carry_check)]
            .into_iter()
            .map(move |(name, poly)| (name, s_word.clone() * poly))
    }

    /// s_decompose_0 for all words
    pub fn s_decompose_0(
        s_decompose_0: Expression<F>,
        lo: Expression<F>,
        hi: Expression<F>,
        word: Expression<F>,
    ) -> Option<(&'static str, Expression<F>)> {
        let check = lo + hi * F::from(1 << 16) - word;
        Some(("s_decompose_0", s_decompose_0 * check))
    }

    /// s_decompose_1 for W_1 to W_13
    /// (3, 4, 11, 14)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_decompose_1(
        s_decompose_1: Expression<F>,
        a: Expression<F>,
        b: Expression<F>,
        c: Expression<F>,
        tag_c: Expression<F>,
        d: Expression<F>,
        tag_d: Expression<F>,
        word: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let decompose_check =
            a + b * F::from(1 << 3) + c * F::from(1 << 7) + d * F::from(1 << 18) + word * (-F::ONE);
        let range_check_tag_c = Gate::range_check(tag_c, 0, 2);
        let range_check_tag_d = Gate::range_check(tag_d, 0, 4);

        [
            ("decompose_check", decompose_check),
            ("range_check_tag_c", range_check_tag_c),
            ("range_check_tag_d", range_check_tag_d),
        ]
        .into_iter()
        .map(move |(name, poly)| (name, s_decompose_1.clone() * poly))
    }

    /// s_decompose_2 for W_14 to W_48
    /// (3, 4, 3, 7, 1, 1, 13)-bit chunks
    #[allow(clippy::many_single_char_names)]
    #[allow(clippy::too_many_arguments)]
    pub fn s_decompose_2(
        s_decompose_2: Expression<F>,
        a: Expression<F>,
        b: Expression<F>,
        c: Expression<F>,
        d: Expression<F>,
        tag_d: Expression<F>,
        e: Expression<F>,
        f: Expression<F>,
        g: Expression<F>,
        tag_g: Expression<F>,
        word: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let decompose_check = a
            + b * F::from(1 << 3)
            + c * F::from(1 << 7)
            + d * F::from(1 << 10)
            + e * F::from(1 << 17)
            + f * F::from(1 << 18)
            + g * F::from(1 << 19)
            + word * (-F::ONE);
        let range_check_tag_d = Gate::range_check(tag_d, 0, 0);
        let range_check_tag_g = Gate::range_check(tag_g, 0, 3);

        [
            ("decompose_check", decompose_check),
            ("range_check_tag_g", range_check_tag_g),
            ("range_check_tag_d", range_check_tag_d),
        ]
        .into_iter()
        .map(move |(name, poly)| (name, s_decompose_2.clone() * poly))
    }

    /// s_decompose_3 for W_49 to W_61
    /// (10, 7, 2, 13)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_decompose_3(
        s_decompose_3: Expression<F>,
        a: Expression<F>,
        tag_a: Expression<F>,
        b: Expression<F>,
        c: Expression<F>,
        d: Expression<F>,
        tag_d: Expression<F>,
        word: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let decompose_check = a
            + b * F::from(1 << 10)
            + c * F::from(1 << 17)
            + d * F::from(1 << 19)
            + word * (-F::ONE);
        let range_check_tag_a = Gate::range_check(tag_a, 0, 1);
        let range_check_tag_d = Gate::range_check(tag_d, 0, 3);

        [
            ("decompose_check", decompose_check),
            ("range_check_tag_a", range_check_tag_a),
            ("range_check_tag_d", range_check_tag_d),
        ]
        .into_iter()
        .map(move |(name, poly)| (name, s_decompose_3.clone() * poly))
    }

    /// b_lo + 2^2 * b_mid = b, on W_[1..49]
    fn check_b(b: Expression<F>, b_lo: Expression<F>, b_hi: Expression<F>) -> Expression<F> {
        let expected_b = b_lo + b_hi * F::from(1 << 2);
        expected_b - b
    }

    /// sigma_0 v1 on W_1 to W_13
    /// (3, 4, 11, 14)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_lower_sigma_0(
        s_lower_sigma_0: Expression<F>,
        spread_r0_even: Expression<F>,
        spread_r0_odd: Expression<F>,
        spread_r1_even: Expression<F>,
        spread_r1_odd: Expression<F>,
        a: Expression<F>,
        spread_a: Expression<F>,
        b: Expression<F>,
        b_lo: Expression<F>,
        spread_b_lo: Expression<F>,
        b_hi: Expression<F>,
        spread_b_hi: Expression<F>,
        spread_c: Expression<F>,
        spread_d: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let check_spread_and_range =
            Gate::two_bit_spread_and_range(b_lo.clone(), spread_b_lo.clone())
                .chain(Gate::two_bit_spread_and_range(
                    b_hi.clone(),
                    spread_b_hi.clone(),
                ))
                .chain(Gate::three_bit_spread_and_range(a, spread_a.clone()));
        let check_b = Self::check_b(b, b_lo, b_hi);
        let spread_witness = spread_r0_even
            + spread_r0_odd * F::from(2)
            + (spread_r1_even + spread_r1_odd * F::from(2)) * F::from(1 << 32);
        let xor_0 = spread_b_lo.clone()
            + spread_b_hi.clone() * F::from(1 << 4)
            + spread_c.clone() * F::from(1 << 8)
            + spread_d.clone() * F::from(1 << 30);
        let xor_1 = spread_c.clone()
            + spread_d.clone() * F::from(1 << 22)
            + spread_a.clone() * F::from(1 << 50)
            + spread_b_lo.clone() * F::from(1 << 56)
            + spread_b_hi.clone() * F::from(1 << 60);
        let xor_2 = spread_d
            + spread_a * F::from(1 << 28)
            + spread_b_lo * F::from(1 << 34)
            + spread_b_hi * F::from(1 << 38)
            + spread_c * F::from(1 << 42);
        let xor = xor_0 + xor_1 + xor_2;

        check_spread_and_range
            .chain(Some(("check_b", check_b)))
            .chain(Some(("lower_sigma_0", spread_witness - xor)))
            .map(move |(name, poly)| (name, s_lower_sigma_0.clone() * poly))
    }

    /// sigma_1 v1 on W_49 to W_61
    /// (10, 7, 2, 13)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_lower_sigma_1(
        s_lower_sigma_1: Expression<F>,
        spread_r0_even: Expression<F>,
        spread_r0_odd: Expression<F>,
        spread_r1_even: Expression<F>,
        spread_r1_odd: Expression<F>,
        spread_a: Expression<F>,
        b: Expression<F>,
        b_lo: Expression<F>,
        spread_b_lo: Expression<F>,
        b_mid: Expression<F>,
        spread_b_mid: Expression<F>,
        b_hi: Expression<F>,
        spread_b_hi: Expression<F>,
        c: Expression<F>,
        spread_c: Expression<F>,
        spread_d: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let check_spread_and_range =
            Gate::two_bit_spread_and_range(b_lo.clone(), spread_b_lo.clone())
                .chain(Gate::two_bit_spread_and_range(
                    b_mid.clone(),
                    spread_b_mid.clone(),
                ))
                .chain(Gate::two_bit_spread_and_range(c, spread_c.clone()))
                .chain(Gate::three_bit_spread_and_range(
                    b_hi.clone(),
                    spread_b_hi.clone(),
                ));
        // b_lo + 2^2 * b_mid + 2^4 * b_hi = b, on W_[49..62]
        let check_b1 = {
            let expected_b = b_lo + b_mid * F::from(1 << 2) + b_hi * F::from(1 << 4);
            expected_b - b
        };
        let spread_witness = spread_r0_even
            + spread_r0_odd * F::from(2)
            + (spread_r1_even + spread_r1_odd * F::from(2)) * F::from(1 << 32);
        let xor_0 = spread_b_lo.clone()
            + spread_b_mid.clone() * F::from(1 << 4)
            + spread_b_hi.clone() * F::from(1 << 8)
            + spread_c.clone() * F::from(1 << 14)
            + spread_d.clone() * F::from(1 << 18);
        let xor_1 = spread_c.clone()
            + spread_d.clone() * F::from(1 << 4)
            + spread_a.clone() * F::from(1 << 30)
            + spread_b_lo.clone() * F::from(1 << 50)
            + spread_b_mid.clone() * F::from(1 << 54)
            + spread_b_hi.clone() * F::from(1 << 58);
        let xor_2 = spread_d
            + spread_a * F::from(1 << 26)
            + spread_b_lo * F::from(1 << 46)
            + spread_b_mid * F::from(1 << 50)
            + spread_b_hi * F::from(1 << 54)
            + spread_c * F::from(1 << 60);
        let xor = xor_0 + xor_1 + xor_2;

        check_spread_and_range
            .chain(Some(("check_b1", check_b1)))
            .chain(Some(("lower_sigma_1", spread_witness - xor)))
            .map(move |(name, poly)| (name, s_lower_sigma_1.clone() * poly))
    }

    /// sigma_0 v2 on W_14 to W_48
    /// (3, 4, 3, 7, 1, 1, 13)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_lower_sigma_0_v2(
        s_lower_sigma_0_v2: Expression<F>,
        spread_r0_even: Expression<F>,
        spread_r0_odd: Expression<F>,
        spread_r1_even: Expression<F>,
        spread_r1_odd: Expression<F>,
        a: Expression<F>,
        spread_a: Expression<F>,
        b: Expression<F>,
        b_lo: Expression<F>,
        spread_b_lo: Expression<F>,
        b_hi: Expression<F>,
        spread_b_hi: Expression<F>,
        c: Expression<F>,
        spread_c: Expression<F>,
        spread_d: Expression<F>,
        spread_e: Expression<F>,
        spread_f: Expression<F>,
        spread_g: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let check_spread_and_range =
            Gate::two_bit_spread_and_range(b_lo.clone(), spread_b_lo.clone())
                .chain(Gate::two_bit_spread_and_range(
                    b_hi.clone(),
                    spread_b_hi.clone(),
                ))
                .chain(Gate::three_bit_spread_and_range(a, spread_a.clone()))
                .chain(Gate::three_bit_spread_and_range(c, spread_c.clone()));
        let check_b = Self::check_b(b, b_lo, b_hi);
        let spread_witness = spread_r0_even
            + spread_r0_odd * F::from(2)
            + (spread_r1_even + spread_r1_odd * F::from(2)) * F::from(1 << 32);
        let xor_0 = spread_b_lo.clone()
            + spread_b_hi.clone() * F::from(1 << 4)
            + spread_c.clone() * F::from(1 << 8)
            + spread_d.clone() * F::from(1 << 14)
            + spread_e.clone() * F::from(1 << 28)
            + spread_f.clone() * F::from(1 << 30)
            + spread_g.clone() * F::from(1 << 32);
        let xor_1 = spread_c.clone()
            + spread_d.clone() * F::from(1 << 6)
            + spread_e.clone() * F::from(1 << 20)
            + spread_f.clone() * F::from(1 << 22)
            + spread_g.clone() * F::from(1 << 24)
            + spread_a.clone() * F::from(1 << 50)
            + spread_b_lo.clone() * F::from(1 << 56)
            + spread_b_hi.clone() * F::from(1 << 60);
        let xor_2 = spread_f
            + spread_g * F::from(1 << 2)
            + spread_a * F::from(1 << 28)
            + spread_b_lo * F::from(1 << 34)
            + spread_b_hi * F::from(1 << 38)
            + spread_c * F::from(1 << 42)
            + spread_d * F::from(1 << 48)
            + spread_e * F::from(1 << 62);
        let xor = xor_0 + xor_1 + xor_2;

        check_spread_and_range
            .chain(Some(("check_b", check_b)))
            .chain(Some(("lower_sigma_0_v2", spread_witness - xor)))
            .map(move |(name, poly)| (name, s_lower_sigma_0_v2.clone() * poly))
    }

    /// sigma_1 v2 on W_14 to W_48
    /// (3, 4, 3, 7, 1, 1, 13)-bit chunks
    #[allow(clippy::too_many_arguments)]
    pub fn s_lower_sigma_1_v2(
        s_lower_sigma_1_v2: Expression<F>,
        spread_r0_even: Expression<F>,
        spread_r0_odd: Expression<F>,
        spread_r1_even: Expression<F>,
        spread_r1_odd: Expression<F>,
        a: Expression<F>,
        spread_a: Expression<F>,
        b: Expression<F>,
        b_lo: Expression<F>,
        spread_b_lo: Expression<F>,
        b_hi: Expression<F>,
        spread_b_hi: Expression<F>,
        c: Expression<F>,
        spread_c: Expression<F>,
        spread_d: Expression<F>,
        spread_e: Expression<F>,
        spread_f: Expression<F>,
        spread_g: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let check_spread_and_range =
            Gate::two_bit_spread_and_range(b_lo.clone(), spread_b_lo.clone())
                .chain(Gate::two_bit_spread_and_range(
                    b_hi.clone(),
                    spread_b_hi.clone(),
                ))
                .chain(Gate::three_bit_spread_and_range(a, spread_a.clone()))
                .chain(Gate::three_bit_spread_and_range(c, spread_c.clone()));
        let check_b = Self::check_b(b, b_lo, b_hi);
        let spread_witness = spread_r0_even
            + spread_r0_odd * F::from(2)
            + (spread_r1_even + spread_r1_odd * F::from(2)) * F::from(1 << 32);
        let xor_0 = spread_d.clone()
            + spread_e.clone() * F::from(1 << 14)
            + spread_f.clone() * F::from(1 << 16)
            + spread_g.clone() * F::from(1 << 18);
        let xor_1 = spread_e.clone()
            + spread_f.clone() * F::from(1 << 2)
            + spread_g.clone() * F::from(1 << 4)
            + spread_a.clone() * F::from(1 << 30)
            + spread_b_lo.clone() * F::from(1 << 36)
            + spread_b_hi.clone() * F::from(1 << 40)
            + spread_c.clone() * F::from(1 << 44)
            + spread_d.clone() * F::from(1 << 50);
        let xor_2 = spread_g
            + spread_a * F::from(1 << 26)
            + spread_b_lo * F::from(1 << 32)
            + spread_b_hi * F::from(1 << 36)
            + spread_c * F::from(1 << 40)
            + spread_d * F::from(1 << 46)
            + spread_e * F::from(1 << 60)
            + spread_f * F::from(1 << 62);
        let xor = xor_0 + xor_1 + xor_2;

        check_spread_and_range
            .chain(Some(("check_b", check_b)))
            .chain(Some(("lower_sigma_1_v2", spread_witness - xor)))
            .map(move |(name, poly)| (name, s_lower_sigma_1_v2.clone() * poly))
    }
}
