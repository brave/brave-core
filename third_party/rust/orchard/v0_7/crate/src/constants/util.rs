/// Takes in an FnMut closure and returns a constant-length array with elements of
/// type `Output`.
pub fn gen_const_array<Output: Copy + Default, const LEN: usize>(
    closure: impl FnMut(usize) -> Output,
) -> [Output; LEN] {
    gen_const_array_with_default(Default::default(), closure)
}

pub(crate) fn gen_const_array_with_default<Output: Copy, const LEN: usize>(
    default_value: Output,
    closure: impl FnMut(usize) -> Output,
) -> [Output; LEN] {
    let mut ret: [Output; LEN] = [default_value; LEN];
    for (bit, val) in ret.iter_mut().zip((0..LEN).map(closure)) {
        *bit = val;
    }
    ret
}
