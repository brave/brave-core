/// Positive difference (f128)
///
/// Determines the positive difference between arguments, returning:
/// * x - y if x > y, or
/// * +0    if x <= y, or
/// * NAN   if either argument is NAN.
///
/// A range error may occur.
#[cfg_attr(all(test, assert_no_panic), no_panic::no_panic)]
pub fn fdimf128(x: f128, y: f128) -> f128 {
    super::generic::fdim(x, y)
}
