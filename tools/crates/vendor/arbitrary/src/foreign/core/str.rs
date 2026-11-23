use {
    crate::{Arbitrary, Result, Unstructured},
    core::str,
};

fn arbitrary_str<'a>(u: &mut Unstructured<'a>, size: usize) -> Result<&'a str> {
    match str::from_utf8(u.peek_bytes(size).unwrap()) {
        Ok(s) => {
            u.bytes(size).unwrap();
            Ok(s)
        }
        Err(e) => {
            let i = e.valid_up_to();
            let valid = u.bytes(i).unwrap();
            // SAFETY:
            // Contract from `str::from_utf8_unchecked`: the bytes must be valid UTF-8.
            // Evidence:
            // - `str::from_utf8` was called on the next `size` peeked bytes from `u`.
            // - The call failed, returning a `Utf8Error` `e`.
            // - `e.valid_up_to()` returns the length of the prefix of the peeked bytes
            //   which is guaranteed to be valid UTF-8.
            // - `u.bytes(i)` consumes and returns exactly this valid prefix (since `u`
            //   was not mutated or consumed between peeking and calling `bytes`).
            // - Therefore, `valid` is guaranteed to contain valid UTF-8.
            let s = unsafe {
                debug_assert!(str::from_utf8(valid).is_ok());
                str::from_utf8_unchecked(valid)
            };
            Ok(s)
        }
    }
}

impl<'a> Arbitrary<'a> for &'a str {
    fn arbitrary(u: &mut Unstructured<'a>) -> Result<Self> {
        let size = u.arbitrary_len::<u8>()?;
        arbitrary_str(u, size)
    }

    fn arbitrary_take_rest(mut u: Unstructured<'a>) -> Result<Self> {
        let size = u.len();
        arbitrary_str(&mut u, size)
    }

    #[inline]
    fn size_hint(_depth: usize) -> (usize, Option<usize>) {
        (0, None)
    }
}
