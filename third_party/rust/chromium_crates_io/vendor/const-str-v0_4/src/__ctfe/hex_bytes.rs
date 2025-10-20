pub struct HexBytes<T>(pub T);

struct Iter<'a> {
    s: &'a [u8],
    i: usize,
}

impl<'a> Iter<'a> {
    const fn new(s: &'a str) -> Self {
        Self {
            s: s.as_bytes(),
            i: 0,
        }
    }

    const fn next(mut self) -> (Self, Option<u8>) {
        let mut i = self.i;
        let s = self.s;

        macro_rules! next {
            () => {{
                if i < s.len() {
                    let b = s[i];
                    i += 1;
                    Some(b)
                } else {
                    None
                }
            }};
        }

        while let Some(b) = next!() {
            let high = match b {
                b'0'..=b'9' => b - b'0',
                b'a'..=b'f' => b - b'a' + 10,
                b'A'..=b'F' => b - b'A' + 10,
                b' ' | b'\r' | b'\n' | b'\t' => continue,
                _ => constfn_panic!("invalid character"),
            };

            let low = match next!() {
                None => constfn_panic!("expected even number of hex characters"),
                Some(b) => match b {
                    b'0'..=b'9' => b - b'0',
                    b'a'..=b'f' => b - b'a' + 10,
                    b'A'..=b'F' => b - b'A' + 10,
                    _ => constfn_panic!("expected hex character"),
                },
            };

            self.i = i;
            let val = (high << 4) | low;
            return (self, Some(val));
        }
        (self, None)
    }
}

impl<'a> HexBytes<&'a str> {
    pub const fn output_len(&self) -> usize {
        let mut ans = 0;
        let mut iter = Iter::new(self.0);

        while let (next, Some(_)) = iter.next() {
            iter = next;
            ans += 1;
        }

        ans
    }

    pub const fn const_eval<const N: usize>(&self) -> [u8; N] {
        let mut buf = [0; N];
        let mut pos = 0;
        let mut iter = Iter::new(self.0);

        while let (next, Some(val)) = iter.next() {
            iter = next;
            buf[pos] = val;
            pos += 1;
        }
        constfn_assert!(pos == N);

        buf
    }
}

impl<'a, 'b> HexBytes<&'b [&'a str]> {
    pub const fn output_len(&self) -> usize {
        let mut i = 0;
        let mut ans = 0;

        while i < self.0.len() {
            let mut iter = Iter::new(self.0[i]);
            while let (next, Some(_)) = iter.next() {
                iter = next;
                ans += 1;
            }
            i += 1;
        }
        ans
    }

    pub const fn const_eval<const N: usize>(&self) -> [u8; N] {
        let mut buf = [0; N];
        let mut pos = 0;

        let mut i = 0;
        while i < self.0.len() {
            let mut iter = Iter::new(self.0[i]);
            while let (next, Some(val)) = iter.next() {
                iter = next;
                buf[pos] = val;
                pos += 1;
            }
            i += 1;
        }
        constfn_assert!(pos == N);
        buf
    }
}

impl<'a, const L: usize> HexBytes<[&'a str; L]> {
    pub const fn output_len(&self) -> usize {
        let ss: &[&str] = &self.0;
        HexBytes(ss).output_len()
    }
    pub const fn const_eval<const N: usize>(&self) -> [u8; N] {
        let ss: &[&str] = &self.0;
        HexBytes(ss).const_eval()
    }
}

/// Converts hexadecimal string slices to a byte array.
///
/// It accepts the following characters in the input string:
///
/// - `'0'...'9'`, `'a'...'f'`, `'A'...'F'` — hex characters which will be used
///     in construction of the output byte array
/// - `' '`, `'\r'`, `'\n'`, `'\t'` — formatting characters which will be
///     ignored
///
///
/// # Examples
/// ```
/// use const_str::hex_bytes as hex;
///
/// const DATA: [u8; 4] = hex!("01020304");
/// assert_eq!(DATA, [1, 2, 3, 4]);
///
/// assert_eq!(hex!("a1 b2 c3 d4"), [0xA1, 0xB2, 0xC3, 0xD4]);
/// assert_eq!(hex!("E5 E6 90 92"), [0xE5, 0xE6, 0x90, 0x92]);
///
/// assert_eq!(hex!(["0a0B", "0C0d"]), [10, 11, 12, 13]);
///
/// const S1: &str = "00010203 04050607 08090a0b 0c0d0e0f";
/// const B1: &[u8] = &hex!(S1);
/// const B2: &[u8] = &hex!([
///     "00010203 04050607", // first half
///     "08090a0b 0c0d0e0f", // second half
/// ]);
///
/// assert_eq!(B1, [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]);
/// assert_eq!(B2, B1);
/// ```
#[macro_export]
macro_rules! hex_bytes {
    ($s: expr) => {{
        const OUTPUT_LEN: usize = $crate::__ctfe::HexBytes($s).output_len();
        const OUTPUT_BUF: [u8; OUTPUT_LEN] = $crate::__ctfe::HexBytes($s).const_eval();
        OUTPUT_BUF
    }};
}
