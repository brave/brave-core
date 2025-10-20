pub struct Equal<T1, T2>(pub T1, pub T2);

impl Equal<&[u8], &[u8]> {
    pub const fn const_eval(&self) -> bool {
        crate::bytes::equal(self.0, self.1)
    }
}

impl<const L1: usize, const L2: usize> Equal<&[u8; L1], &[u8; L2]> {
    pub const fn const_eval(&self) -> bool {
        crate::bytes::equal(self.0, self.1)
    }
}

impl Equal<&str, &str> {
    pub const fn const_eval(&self) -> bool {
        crate::str::equal(self.0, self.1)
    }
}

/// Checks that two strings are equal.
///
/// # Examples
///
/// ```
/// const A: &str = "hello";
/// const B: &str = "world";
/// const C: &str = "hello";
/// const EQ_AB: bool = const_str::equal!(A, B);
/// const EQ_AC: bool = const_str::equal!(A, C);
/// assert_eq!([EQ_AB, EQ_AC], [false, true]);
///
#[macro_export]
macro_rules! equal {
    ($lhs: expr, $rhs: expr) => {
        $crate::__ctfe::Equal($lhs, $rhs).const_eval()
    };
}
