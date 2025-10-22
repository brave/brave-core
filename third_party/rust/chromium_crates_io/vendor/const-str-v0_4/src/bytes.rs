use core::ops::Range;

pub const fn clone<const N: usize>(bytes: &[u8]) -> [u8; N] {
    constfn_assert!(bytes.len() == N);
    let mut buf = [0; N];
    let mut i = 0;
    while i < bytes.len() {
        buf[i] = bytes[i];
        i += 1;
    }
    buf
}

pub const fn equal(lhs: &[u8], rhs: &[u8]) -> bool {
    if lhs.len() != rhs.len() {
        return false;
    }
    let mut i = 0;
    while i < lhs.len() {
        if lhs[i] != rhs[i] {
            return false;
        }
        i += 1;
    }
    true
}

pub const fn subslice<T>(mut s: &[T], range: Range<usize>) -> &[T] {
    constfn_assert!(range.end >= range.start);

    let mut i = 0;
    let mut j = s.len();

    while i < range.start {
        match s {
            [_, xs @ ..] => {
                i += 1;
                s = xs;
            }
            _ => break,
        }
    }

    while j > range.end {
        match s {
            [xs @ .., _] => {
                j -= 1;
                s = xs;
            }
            _ => break,
        }
    }

    constfn_assert!(i == range.start);
    constfn_assert!(j == range.end);
    constfn_assert!(s.len() == j - i);
    s
}

#[test]
fn test_subslice() {
    let buf = b"abcdefgh";
    assert_eq!(subslice(buf, 0..0), &[]);
    assert_eq!(subslice(buf, 0..1), b"a");
    assert_eq!(subslice(buf, 1..3), b"bc");
    assert_eq!(subslice(buf, 7..8), b"h");

    assert_eq!(subslice::<u8>(&[], 0..0), &[]);
}

pub const fn merge<const N: usize>(mut buf: [u8; N], bytes: &[u8]) -> [u8; N] {
    constfn_assert!(N <= bytes.len());
    let mut i = 0;
    while i < bytes.len() {
        buf[i] = bytes[i];
        i += 1;
    }
    buf
}

pub const fn reversed<const N: usize>(mut arr: [u8; N]) -> [u8; N] {
    let mut i = 0;
    while i * 2 < N {
        let a = arr[i];
        let b = arr[N - 1 - i];
        arr[i] = b;
        arr[N - 1 - i] = a;
        i += 1;
    }
    arr
}

#[test]
fn test_reversed() {
    let arr = [0, 1];
    assert_eq!(reversed(arr), [1, 0]);

    let arr = [0, 1, 2];
    assert_eq!(reversed(arr), [2, 1, 0]);
}

pub const fn advance(mut s: &[u8], count: usize) -> &[u8] {
    constfn_assert!(count <= s.len());
    let mut i = 0;
    while i < count {
        match s {
            [_, xs @ ..] => s = xs,
            _ => break,
        }
        i += 1;
    }
    constfn_assert!(i == count);
    s
}

pub const fn contains(haystack: &[u8], needle: &[u8]) -> bool {
    let haystack_len = haystack.len();
    let needle_len = needle.len();

    let mut i = 0;
    while i < haystack_len {
        let mut j = 0;
        while j < needle_len && i + j < haystack_len {
            if haystack[i + j] != needle[j] {
                break;
            }
            j += 1;
        }
        if j == needle_len {
            return true;
        }
        i += 1;
    }

    false
}

#[test]
fn test_contains() {
    macro_rules! test_contains {
        (true, $haystack: expr, $needle: expr) => {
            assert!(contains($haystack.as_ref(), $needle.as_ref()));
        };
        (false, $haystack: expr, $needle: expr) => {
            assert!(!contains($haystack.as_ref(), $needle.as_ref()));
        };
    }

    let buf = b"abcdefgh";
    test_contains!(true, buf, b"");
    test_contains!(true, buf, b"a");
    test_contains!(true, buf, b"ef");
    test_contains!(false, buf, b"xyz");

    test_contains!(true, "asd", "");
    test_contains!(true, "asd", "a");
    test_contains!(true, "asdf", "sd");
    test_contains!(false, "", "a");
    test_contains!(false, "asd", "abcd");

    test_contains!(true, "唐可可", "可");
    test_contains!(true, "Liyuu", "i");
    test_contains!(false, "Liyuu", "我");
}

pub const fn starts_with(haystack: &[u8], needle: &[u8]) -> bool {
    let haystack_len = haystack.len();
    let needle_len = needle.len();

    if needle_len > haystack_len {
        return false;
    }

    let mut i = 0;
    while i < needle_len {
        if haystack[i] != needle[i] {
            break;
        }
        i += 1
    }

    i == needle_len
}

#[test]
fn test_starts_with() {
    assert!(starts_with(b"", b""));
    assert!(starts_with(b"a", b""));
    assert!(starts_with(b"a", b"a"));
    assert!(!starts_with(b"", b"a"));
    assert!(!starts_with(b"ba", b"a"));
}

pub const fn ends_with(haystack: &[u8], needle: &[u8]) -> bool {
    let haystack_len = haystack.len();
    let needle_len = needle.len();

    if needle_len > haystack_len {
        return false;
    }

    let mut i = 0;
    while i < needle_len {
        if haystack[haystack_len - needle_len + i] != needle[i] {
            break;
        }
        i += 1
    }

    i == needle_len
}

#[test]
fn test_ends_with() {
    assert!(ends_with(b"", b""));
    assert!(ends_with(b"a", b""));
    assert!(ends_with(b"a", b"a"));
    assert!(!ends_with(b"", b"a"));
    assert!(!ends_with(b"ab", b"a"));
}
