use {
    crate::{Arbitrary, Result, Unstructured},
    std::ffi::CString,
};

impl<'a> Arbitrary<'a> for CString {
    fn arbitrary(u: &mut Unstructured<'a>) -> Result<Self> {
        <Vec<u8> as Arbitrary>::arbitrary(u).map(|mut x| {
            x.retain(|&c| c != 0);
            // SAFETY:
            // Contract from `CString::from_vec_unchecked`: the vector must not contain
            // any interior nul (zero) bytes.
            // Evidence: `x.retain(|&c| c != 0)` removes all bytes equal to `0` from the
            // vector `x`. Consequently, `x` contains no nul bytes, which guarantees
            // it has no interior nul bytes.
            unsafe { Self::from_vec_unchecked(x) }
        })
    }

    #[inline]
    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        <Vec<u8> as Arbitrary>::size_hint(depth)
    }
}
