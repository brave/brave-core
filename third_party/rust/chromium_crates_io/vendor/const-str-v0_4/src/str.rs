pub const fn equal(lhs: &str, rhs: &str) -> bool {
    crate::bytes::equal(lhs.as_bytes(), rhs.as_bytes())
}

// https://github.com/rust-lang/rust/issues/89259
#[allow(unsafe_code, clippy::transmute_int_to_char)]
pub const unsafe fn char_from_u32(x: u32) -> char {
    core::mem::transmute(x)
}

pub const fn contains(haystack: &str, needle: &str) -> bool {
    crate::bytes::contains(haystack.as_bytes(), needle.as_bytes())
}

pub const fn starts_with(haystack: &str, needle: &str) -> bool {
    crate::bytes::starts_with(haystack.as_bytes(), needle.as_bytes())
}

pub const fn ends_with(haystack: &str, needle: &str) -> bool {
    crate::bytes::ends_with(haystack.as_bytes(), needle.as_bytes())
}

#[allow(unsafe_code)]
pub const fn next_match<'h>(haystack: &'h str, needle: &str) -> Option<(usize, &'h str)> {
    constfn_assert!(!needle.is_empty());

    let lhs = haystack.as_bytes();
    let rhs = needle.as_bytes();

    let mut i = 0;
    while i + rhs.len() <= lhs.len() {
        let mut j = 0;
        while j < rhs.len() {
            if lhs[i + j] != rhs[j] {
                break;
            }
            j += 1;
        }
        if j == rhs.len() {
            let remain = crate::bytes::advance(lhs, i + rhs.len());
            let remain = unsafe { core::str::from_utf8_unchecked(remain) };
            return Some((i, remain));
        }

        i += 1;
    }

    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_next_match() {
        assert_eq!(next_match("abc", "ab"), Some((0, "c")));
        assert_eq!(next_match("abc", "bc"), Some((1, "")));
        assert_eq!(next_match("abc", "c"), Some((2, "")));
        assert_eq!(next_match("abc", "d"), None);
    }
}
