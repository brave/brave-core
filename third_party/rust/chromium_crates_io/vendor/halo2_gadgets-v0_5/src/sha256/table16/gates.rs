use group::ff::PrimeField;
use halo2_proofs::plonk::Expression;

pub struct Gate<F: PrimeField>(pub Expression<F>);

impl<F: PrimeField> Gate<F> {
    fn ones() -> Expression<F> {
        Expression::Constant(F::ONE)
    }

    // Helper gates
    fn lagrange_interpolate(
        var: Expression<F>,
        points: Vec<u16>,
        evals: Vec<u32>,
    ) -> (F, Expression<F>) {
        assert_eq!(points.len(), evals.len());
        let deg = points.len();

        fn factorial(n: u64) -> u64 {
            if n < 2 {
                1
            } else {
                n * factorial(n - 1)
            }
        }

        // Scale the whole expression by factor to avoid divisions
        let factor = factorial((deg - 1) as u64);

        let numerator = |var: Expression<F>, eval: u32, idx: u64| {
            let mut expr = Self::ones();
            for i in 0..deg {
                let i = i as u64;
                if i != idx {
                    expr = expr * (Self::ones() * (-F::ONE) * F::from(i) + var.clone());
                }
            }
            expr * F::from(u64::from(eval))
        };
        let denominator = |idx: i32| {
            let mut denom: i32 = 1;
            for i in 0..deg {
                let i = i as i32;
                if i != idx {
                    denom *= idx - i
                }
            }
            if denom < 0 {
                -F::ONE * F::from(factor / (-denom as u64))
            } else {
                F::from(factor / (denom as u64))
            }
        };

        let mut expr = Self::ones() * F::ZERO;
        for ((idx, _), eval) in points.iter().enumerate().zip(evals.iter()) {
            expr = expr + numerator(var.clone(), *eval, idx as u64) * denominator(idx as i32)
        }

        (F::from(factor), expr)
    }

    pub fn range_check(value: Expression<F>, lower_range: u64, upper_range: u64) -> Expression<F> {
        let mut expr = Self::ones();
        for i in lower_range..(upper_range + 1) {
            expr = expr * (Self::ones() * (-F::ONE) * F::from(i) + value.clone())
        }
        expr
    }

    /// Spread and range check on 2-bit word
    pub fn two_bit_spread_and_range(
        dense: Expression<F>,
        spread: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let two_bit_spread = |dense: Expression<F>, spread: Expression<F>| {
            let (factor, lagrange_poly) = Self::lagrange_interpolate(
                dense,
                vec![0b00, 0b01, 0b10, 0b11],
                vec![0b0000, 0b0001, 0b0100, 0b0101],
            );

            lagrange_poly - spread * factor
        };

        std::iter::empty()
            .chain(Some((
                "two_bit_range_check",
                Self::range_check(dense.clone(), 0, (1 << 2) - 1),
            )))
            .chain(Some((
                "two_bit_spread_check",
                two_bit_spread(dense, spread),
            )))
    }

    /// Spread and range check on 3-bit word
    pub fn three_bit_spread_and_range(
        dense: Expression<F>,
        spread: Expression<F>,
    ) -> impl Iterator<Item = (&'static str, Expression<F>)> {
        let three_bit_spread = |dense: Expression<F>, spread: Expression<F>| {
            let (factor, lagrange_poly) = Self::lagrange_interpolate(
                dense,
                vec![0b000, 0b001, 0b010, 0b011, 0b100, 0b101, 0b110, 0b111],
                vec![
                    0b000000, 0b000001, 0b000100, 0b000101, 0b010000, 0b010001, 0b010100, 0b010101,
                ],
            );

            lagrange_poly - spread * factor
        };

        std::iter::empty()
            .chain(Some((
                "three_bit_range_check",
                Self::range_check(dense.clone(), 0, (1 << 3) - 1),
            )))
            .chain(Some((
                "three_bit_spread_check",
                three_bit_spread(dense, spread),
            )))
    }
}
