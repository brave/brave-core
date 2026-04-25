use std::ops::{Add, AddAssign, Mul, MulAssign, Neg, Sub, SubAssign};

use group::ff::Field;

/// A value assigned to a cell within a circuit.
///
/// Stored as a fraction, so the backend can use batch inversion.
///
/// A denominator of zero maps to an assigned value of zero.
#[derive(Clone, Copy, Debug)]
pub enum Assigned<F> {
    /// The field element zero.
    Zero,
    /// A value that does not require inversion to evaluate.
    Trivial(F),
    /// A value stored as a fraction to enable batch inversion.
    Rational(F, F),
}

impl<F: Field> From<&Assigned<F>> for Assigned<F> {
    fn from(val: &Assigned<F>) -> Self {
        *val
    }
}

impl<F: Field> From<&F> for Assigned<F> {
    fn from(numerator: &F) -> Self {
        Assigned::Trivial(*numerator)
    }
}

impl<F: Field> From<F> for Assigned<F> {
    fn from(numerator: F) -> Self {
        Assigned::Trivial(numerator)
    }
}

impl<F: Field> From<(F, F)> for Assigned<F> {
    fn from((numerator, denominator): (F, F)) -> Self {
        Assigned::Rational(numerator, denominator)
    }
}

impl<F: Field> PartialEq for Assigned<F> {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            // At least one side is directly zero.
            (Self::Zero, Self::Zero) => true,
            (Self::Zero, x) | (x, Self::Zero) => x.is_zero_vartime(),

            // One side is x/0 which maps to zero.
            (Self::Rational(_, denominator), x) | (x, Self::Rational(_, denominator))
                if denominator.is_zero_vartime() =>
            {
                x.is_zero_vartime()
            }

            // Okay, we need to do some actual math...
            (Self::Trivial(lhs), Self::Trivial(rhs)) => lhs == rhs,
            (Self::Trivial(x), Self::Rational(numerator, denominator))
            | (Self::Rational(numerator, denominator), Self::Trivial(x)) => {
                &(*x * denominator) == numerator
            }
            (
                Self::Rational(lhs_numerator, lhs_denominator),
                Self::Rational(rhs_numerator, rhs_denominator),
            ) => *lhs_numerator * rhs_denominator == *lhs_denominator * rhs_numerator,
        }
    }
}

impl<F: Field> Eq for Assigned<F> {}

impl<F: Field> Neg for Assigned<F> {
    type Output = Assigned<F>;
    fn neg(self) -> Self::Output {
        match self {
            Self::Zero => Self::Zero,
            Self::Trivial(numerator) => Self::Trivial(-numerator),
            Self::Rational(numerator, denominator) => Self::Rational(-numerator, denominator),
        }
    }
}

impl<F: Field> Neg for &Assigned<F> {
    type Output = Assigned<F>;
    fn neg(self) -> Self::Output {
        -*self
    }
}

impl<F: Field> Add for Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: Assigned<F>) -> Assigned<F> {
        match (self, rhs) {
            // One side is directly zero.
            (Self::Zero, _) => rhs,
            (_, Self::Zero) => self,

            // One side is x/0 which maps to zero.
            (Self::Rational(_, denominator), other) | (other, Self::Rational(_, denominator))
                if denominator.is_zero_vartime() =>
            {
                other
            }

            // Okay, we need to do some actual math...
            (Self::Trivial(lhs), Self::Trivial(rhs)) => Self::Trivial(lhs + rhs),
            (Self::Rational(numerator, denominator), Self::Trivial(other))
            | (Self::Trivial(other), Self::Rational(numerator, denominator)) => {
                Self::Rational(numerator + denominator * other, denominator)
            }
            (
                Self::Rational(lhs_numerator, lhs_denominator),
                Self::Rational(rhs_numerator, rhs_denominator),
            ) => Self::Rational(
                lhs_numerator * rhs_denominator + lhs_denominator * rhs_numerator,
                lhs_denominator * rhs_denominator,
            ),
        }
    }
}

impl<F: Field> Add<F> for Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: F) -> Assigned<F> {
        self + Self::Trivial(rhs)
    }
}

impl<F: Field> Add<F> for &Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: F) -> Assigned<F> {
        *self + rhs
    }
}

impl<F: Field> Add<&Assigned<F>> for Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: &Self) -> Assigned<F> {
        self + *rhs
    }
}

impl<F: Field> Add<Assigned<F>> for &Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: Assigned<F>) -> Assigned<F> {
        *self + rhs
    }
}

impl<F: Field> Add<&Assigned<F>> for &Assigned<F> {
    type Output = Assigned<F>;
    fn add(self, rhs: &Assigned<F>) -> Assigned<F> {
        *self + *rhs
    }
}

impl<F: Field> AddAssign for Assigned<F> {
    fn add_assign(&mut self, rhs: Self) {
        *self = *self + rhs;
    }
}

impl<F: Field> AddAssign<&Assigned<F>> for Assigned<F> {
    fn add_assign(&mut self, rhs: &Self) {
        *self = *self + rhs;
    }
}

impl<F: Field> Sub for Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: Assigned<F>) -> Assigned<F> {
        self + (-rhs)
    }
}

impl<F: Field> Sub<F> for Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: F) -> Assigned<F> {
        self + (-rhs)
    }
}

impl<F: Field> Sub<F> for &Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: F) -> Assigned<F> {
        *self - rhs
    }
}

impl<F: Field> Sub<&Assigned<F>> for Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: &Self) -> Assigned<F> {
        self - *rhs
    }
}

impl<F: Field> Sub<Assigned<F>> for &Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: Assigned<F>) -> Assigned<F> {
        *self - rhs
    }
}

impl<F: Field> Sub<&Assigned<F>> for &Assigned<F> {
    type Output = Assigned<F>;
    fn sub(self, rhs: &Assigned<F>) -> Assigned<F> {
        *self - *rhs
    }
}

impl<F: Field> SubAssign for Assigned<F> {
    fn sub_assign(&mut self, rhs: Self) {
        *self = *self - rhs;
    }
}

impl<F: Field> SubAssign<&Assigned<F>> for Assigned<F> {
    fn sub_assign(&mut self, rhs: &Self) {
        *self = *self - rhs;
    }
}

impl<F: Field> Mul for Assigned<F> {
    type Output = Assigned<F>;
    fn mul(self, rhs: Assigned<F>) -> Assigned<F> {
        match (self, rhs) {
            (Self::Zero, _) | (_, Self::Zero) => Self::Zero,
            (Self::Trivial(lhs), Self::Trivial(rhs)) => Self::Trivial(lhs * rhs),
            (Self::Rational(numerator, denominator), Self::Trivial(other))
            | (Self::Trivial(other), Self::Rational(numerator, denominator)) => {
                Self::Rational(numerator * other, denominator)
            }
            (
                Self::Rational(lhs_numerator, lhs_denominator),
                Self::Rational(rhs_numerator, rhs_denominator),
            ) => Self::Rational(
                lhs_numerator * rhs_numerator,
                lhs_denominator * rhs_denominator,
            ),
        }
    }
}

impl<F: Field> Mul<F> for Assigned<F> {
    type Output = Assigned<F>;
    fn mul(self, rhs: F) -> Assigned<F> {
        self * Self::Trivial(rhs)
    }
}

impl<F: Field> Mul<F> for &Assigned<F> {
    type Output = Assigned<F>;
    fn mul(self, rhs: F) -> Assigned<F> {
        *self * rhs
    }
}

impl<F: Field> Mul<&Assigned<F>> for Assigned<F> {
    type Output = Assigned<F>;
    fn mul(self, rhs: &Assigned<F>) -> Assigned<F> {
        self * *rhs
    }
}

impl<F: Field> MulAssign for Assigned<F> {
    fn mul_assign(&mut self, rhs: Self) {
        *self = *self * rhs;
    }
}

impl<F: Field> MulAssign<&Assigned<F>> for Assigned<F> {
    fn mul_assign(&mut self, rhs: &Self) {
        *self = *self * rhs;
    }
}

impl<F: Field> Assigned<F> {
    /// Returns the numerator.
    pub fn numerator(&self) -> F {
        match self {
            Self::Zero => F::ZERO,
            Self::Trivial(x) => *x,
            Self::Rational(numerator, _) => *numerator,
        }
    }

    /// Returns the denominator, if non-trivial.
    pub fn denominator(&self) -> Option<F> {
        match self {
            Self::Zero => None,
            Self::Trivial(_) => None,
            Self::Rational(_, denominator) => Some(*denominator),
        }
    }

    /// Returns true iff this element is zero.
    pub fn is_zero_vartime(&self) -> bool {
        match self {
            Self::Zero => true,
            Self::Trivial(x) => x.is_zero_vartime(),
            // Assigned maps x/0 -> 0.
            Self::Rational(numerator, denominator) => {
                numerator.is_zero_vartime() || denominator.is_zero_vartime()
            }
        }
    }

    /// Doubles this element.
    #[must_use]
    pub fn double(&self) -> Self {
        match self {
            Self::Zero => Self::Zero,
            Self::Trivial(x) => Self::Trivial(x.double()),
            Self::Rational(numerator, denominator) => {
                Self::Rational(numerator.double(), *denominator)
            }
        }
    }

    /// Squares this element.
    #[must_use]
    pub fn square(&self) -> Self {
        match self {
            Self::Zero => Self::Zero,
            Self::Trivial(x) => Self::Trivial(x.square()),
            Self::Rational(numerator, denominator) => {
                Self::Rational(numerator.square(), denominator.square())
            }
        }
    }

    /// Cubes this element.
    #[must_use]
    pub fn cube(&self) -> Self {
        self.square() * self
    }

    /// Inverts this assigned value (taking the inverse of zero to be zero).
    pub fn invert(&self) -> Self {
        match self {
            Self::Zero => Self::Zero,
            Self::Trivial(x) => Self::Rational(F::ONE, *x),
            Self::Rational(numerator, denominator) => Self::Rational(*denominator, *numerator),
        }
    }

    /// Evaluates this assigned value directly, performing an unbatched inversion if
    /// necessary.
    ///
    /// If the denominator is zero, this returns zero.
    pub fn evaluate(self) -> F {
        match self {
            Self::Zero => F::ZERO,
            Self::Trivial(x) => x,
            Self::Rational(numerator, denominator) => {
                if denominator == F::ONE {
                    numerator
                } else {
                    numerator * denominator.invert().unwrap_or(F::ZERO)
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use group::ff::Field;
    use pasta_curves::Fp;

    use super::Assigned;
    // We use (numerator, denominator) in the comments below to denote a rational.
    #[test]
    fn add_trivial_to_inv0_rational() {
        // a = 2
        // b = (1,0)
        let a = Assigned::Trivial(Fp::from(2));
        let b = Assigned::Rational(Fp::ONE, Fp::ZERO);

        // 2 + (1,0) = 2 + 0 = 2
        // This fails if addition is implemented using normal rules for rationals.
        assert_eq!((a + b).evaluate(), a.evaluate());
        assert_eq!((b + a).evaluate(), a.evaluate());
    }

    #[test]
    fn add_rational_to_inv0_rational() {
        // a = (1,2)
        // b = (1,0)
        let a = Assigned::Rational(Fp::ONE, Fp::from(2));
        let b = Assigned::Rational(Fp::ONE, Fp::ZERO);

        // (1,2) + (1,0) = (1,2) + 0 = (1,2)
        // This fails if addition is implemented using normal rules for rationals.
        assert_eq!((a + b).evaluate(), a.evaluate());
        assert_eq!((b + a).evaluate(), a.evaluate());
    }

    #[test]
    fn sub_trivial_from_inv0_rational() {
        // a = 2
        // b = (1,0)
        let a = Assigned::Trivial(Fp::from(2));
        let b = Assigned::Rational(Fp::ONE, Fp::ZERO);

        // (1,0) - 2 = 0 - 2 = -2
        // This fails if subtraction is implemented using normal rules for rationals.
        assert_eq!((b - a).evaluate(), (-a).evaluate());

        // 2 - (1,0) = 2 - 0 = 2
        assert_eq!((a - b).evaluate(), a.evaluate());
    }

    #[test]
    fn sub_rational_from_inv0_rational() {
        // a = (1,2)
        // b = (1,0)
        let a = Assigned::Rational(Fp::ONE, Fp::from(2));
        let b = Assigned::Rational(Fp::ONE, Fp::ZERO);

        // (1,0) - (1,2) = 0 - (1,2) = -(1,2)
        // This fails if subtraction is implemented using normal rules for rationals.
        assert_eq!((b - a).evaluate(), (-a).evaluate());

        // (1,2) - (1,0) = (1,2) - 0 = (1,2)
        assert_eq!((a - b).evaluate(), a.evaluate());
    }

    #[test]
    fn mul_rational_by_inv0_rational() {
        // a = (1,2)
        // b = (1,0)
        let a = Assigned::Rational(Fp::ONE, Fp::from(2));
        let b = Assigned::Rational(Fp::ONE, Fp::ZERO);

        // (1,2) * (1,0) = (1,2) * 0 = 0
        assert_eq!((a * b).evaluate(), Fp::ZERO);

        // (1,0) * (1,2) = 0 * (1,2) = 0
        assert_eq!((b * a).evaluate(), Fp::ZERO);
    }
}

#[cfg(test)]
mod proptests {
    use std::{
        cmp,
        ops::{Add, Mul, Neg, Sub},
    };

    use group::ff::Field;
    use pasta_curves::Fp;
    use proptest::{collection::vec, prelude::*, sample::select};

    use super::Assigned;

    trait UnaryOperand: Neg<Output = Self> {
        fn double(&self) -> Self;
        fn square(&self) -> Self;
        fn cube(&self) -> Self;
        fn inv0(&self) -> Self;
    }

    impl<F: Field> UnaryOperand for F {
        fn double(&self) -> Self {
            self.double()
        }

        fn square(&self) -> Self {
            self.square()
        }

        fn cube(&self) -> Self {
            self.cube()
        }

        fn inv0(&self) -> Self {
            self.invert().unwrap_or(F::ZERO)
        }
    }

    impl<F: Field> UnaryOperand for Assigned<F> {
        fn double(&self) -> Self {
            self.double()
        }

        fn square(&self) -> Self {
            self.square()
        }

        fn cube(&self) -> Self {
            self.cube()
        }

        fn inv0(&self) -> Self {
            self.invert()
        }
    }

    #[derive(Clone, Debug)]
    enum UnaryOperator {
        Neg,
        Double,
        Square,
        Cube,
        Inv0,
    }

    const UNARY_OPERATORS: &[UnaryOperator] = &[
        UnaryOperator::Neg,
        UnaryOperator::Double,
        UnaryOperator::Square,
        UnaryOperator::Cube,
        UnaryOperator::Inv0,
    ];

    impl UnaryOperator {
        fn apply<F: UnaryOperand>(&self, a: F) -> F {
            match self {
                Self::Neg => -a,
                Self::Double => a.double(),
                Self::Square => a.square(),
                Self::Cube => a.cube(),
                Self::Inv0 => a.inv0(),
            }
        }
    }

    trait BinaryOperand: Sized + Add<Output = Self> + Sub<Output = Self> + Mul<Output = Self> {}
    impl<F: Field> BinaryOperand for F {}
    impl<F: Field> BinaryOperand for Assigned<F> {}

    #[derive(Clone, Debug)]
    enum BinaryOperator {
        Add,
        Sub,
        Mul,
    }

    const BINARY_OPERATORS: &[BinaryOperator] = &[
        BinaryOperator::Add,
        BinaryOperator::Sub,
        BinaryOperator::Mul,
    ];

    impl BinaryOperator {
        fn apply<F: BinaryOperand>(&self, a: F, b: F) -> F {
            match self {
                Self::Add => a + b,
                Self::Sub => a - b,
                Self::Mul => a * b,
            }
        }
    }

    #[derive(Clone, Debug)]
    enum Operator {
        Unary(UnaryOperator),
        Binary(BinaryOperator),
    }

    prop_compose! {
        /// Use narrow that can be easily reduced.
        fn arb_element()(val in any::<u64>()) -> Fp {
            Fp::from(val)
        }
    }

    prop_compose! {
        fn arb_trivial()(element in arb_element()) -> Assigned<Fp> {
            Assigned::Trivial(element)
        }
    }

    prop_compose! {
        /// Generates half of the denominators as zero to represent a deferred inversion.
        fn arb_rational()(
            numerator in arb_element(),
            denominator in prop_oneof![
                1 => Just(Fp::ZERO),
                2 => arb_element(),
            ],
        ) -> Assigned<Fp> {
            Assigned::Rational(numerator, denominator)
        }
    }

    prop_compose! {
        fn arb_operators(num_unary: usize, num_binary: usize)(
            unary in vec(select(UNARY_OPERATORS), num_unary),
            binary in vec(select(BINARY_OPERATORS), num_binary),
        ) -> Vec<Operator> {
            unary.into_iter()
                .map(Operator::Unary)
                .chain(binary.into_iter().map(Operator::Binary))
                .collect()
        }
    }

    prop_compose! {
        fn arb_testcase()(
            num_unary in 0usize..5,
            num_binary in 0usize..5,
        )(
            values in vec(
                prop_oneof![
                    1 => Just(Assigned::Zero),
                    2 => arb_trivial(),
                    2 => arb_rational(),
                ],
                // Ensure that:
                // - we have at least one value to apply unary operators to.
                // - we can apply every binary operator pairwise sequentially.
                cmp::max(usize::from(num_unary > 0), num_binary + 1)),
            operations in arb_operators(num_unary, num_binary).prop_shuffle(),
        ) -> (Vec<Assigned<Fp>>, Vec<Operator>) {
            (values, operations)
        }
    }

    proptest! {
        #[test]
        fn operation_commutativity((values, operations) in arb_testcase()) {
            // Evaluate the values at the start.
            let elements: Vec<_> = values.iter().cloned().map(|v| v.evaluate()).collect();

            // Apply the operations to both the deferred and evaluated values.
            fn evaluate<F: UnaryOperand + BinaryOperand>(
                items: Vec<F>,
                operators: &[Operator],
            ) -> F {
                let mut ops = operators.iter();

                // Process all binary operators. We are guaranteed to have exactly as many
                // binary operators as we need calls to the reduction closure.
                let mut res = items.into_iter().reduce(|mut a, b| loop {
                    match ops.next() {
                        Some(Operator::Unary(op)) => a = op.apply(a),
                        Some(Operator::Binary(op)) => break op.apply(a, b),
                        None => unreachable!(),
                    }
                }).unwrap();

                // Process any unary operators that weren't handled in the reduce() call
                // above (either if we only had one item, or there were unary operators
                // after the last binary operator). We are guaranteed to have no binary
                // operators remaining at this point.
                loop {
                    match ops.next() {
                        Some(Operator::Unary(op)) => res = op.apply(res),
                        Some(Operator::Binary(_)) => unreachable!(),
                        None => break res,
                    }
                }
            }
            let deferred_result = evaluate(values, &operations);
            let evaluated_result = evaluate(elements, &operations);

            // The two should be equal, i.e. deferred inversion should commute with the
            // list of operations.
            assert_eq!(deferred_result.evaluate(), evaluated_result);
        }
    }
}
