#![allow(unsafe_code)]

use core::mem;

use crate::printable::is_printable;

pub struct CharEncodeUtf8 {
    buf: [u8; 4],
    len: u8,
}

impl CharEncodeUtf8 {
    /// Copied from [char::encode_utf8](https://github.com/rust-lang/rust/blob/0273e3bce7a0ce49e96a9662163e2380cb87e0be/library/core/src/char/methods.rs#L1600-L1645)
    pub const fn new(ch: char) -> Self {
        // UTF-8 ranges and tags for encoding characters
        const TAG_CONT: u8 = 0b1000_0000;
        const TAG_TWO_B: u8 = 0b1100_0000;
        const TAG_THREE_B: u8 = 0b1110_0000;
        const TAG_FOUR_B: u8 = 0b1111_0000;

        let mut buf = [0; 4];
        let len = ch.len_utf8();
        let code = ch as u32;

        match len {
            1 => {
                buf[0] = code as u8;
            }
            2 => {
                buf[0] = (code >> 6 & 0x1F) as u8 | TAG_TWO_B;
                buf[1] = (code & 0x3F) as u8 | TAG_CONT;
            }
            3 => {
                buf[0] = (code >> 12 & 0x0F) as u8 | TAG_THREE_B;
                buf[1] = (code >> 6 & 0x3F) as u8 | TAG_CONT;
                buf[2] = (code & 0x3F) as u8 | TAG_CONT;
            }
            4 => {
                buf[0] = (code >> 18 & 0x07) as u8 | TAG_FOUR_B;
                buf[1] = (code >> 12 & 0x3F) as u8 | TAG_CONT;
                buf[2] = (code >> 6 & 0x3F) as u8 | TAG_CONT;
                buf[3] = (code & 0x3F) as u8 | TAG_CONT;
            }
            _ => {}
        };

        CharEncodeUtf8 {
            buf,
            len: len as u8,
        }
    }

    pub const fn as_bytes(&self) -> &[u8] {
        crate::bytes::subslice(&self.buf, 0..self.len as usize)
    }

    #[cfg(test)]
    pub fn as_str(&self) -> &str {
        unsafe { core::str::from_utf8_unchecked(&self.buf[..self.len as usize]) }
    }
}

#[test]
fn test_char_encode_utf8() {
    macro_rules! test_char_encode_utf8 {
        ($ch: expr) => {{
            let e = CharEncodeUtf8::new($ch);
            let output = e.as_str();
            let mut ans = [0; 4];
            let ans = $ch.encode_utf8(&mut ans);
            assert_eq!(output, ans);
        }};
    }

    test_char_encode_utf8!('\0');
    test_char_encode_utf8!('我');
    test_char_encode_utf8!('\u{10ffff}');
}

pub struct CharEscapeUnicode {
    buf: [u8; 10],
    len: u8,
}

impl CharEscapeUnicode {
    const unsafe fn from_code_point(code: u32) -> Self {
        let mut hex_buf = [0; 10];
        let mut hex_pos = 0;

        let mut x = code;
        loop {
            hex_buf[hex_pos] = crate::ascii::num_to_hex_digit((x as u8) & 0x0f);
            hex_pos += 1;
            x >>= 4;
            if x == 0 {
                break;
            }
        }

        let mut buf = [b'\\', b'u', b'{', 0, 0, 0, 0, 0, 0, 0];
        let mut pos = 3;

        while hex_pos > 0 {
            hex_pos -= 1;
            buf[pos] = hex_buf[hex_pos];
            pos += 1;
        }

        buf[pos] = b'}';
        pos += 1;

        Self {
            buf,
            len: pos as u8,
        }
    }

    pub const fn new(ch: char) -> Self {
        unsafe { Self::from_code_point(ch as u32) }
    }

    #[cfg(test)]
    pub fn as_str(&self) -> &str {
        unsafe { core::str::from_utf8_unchecked(&self.buf[..self.len as usize]) }
    }
}

#[test]
fn test_char_escape_unicode() {
    macro_rules! test_char_escape_unicode {
        ($ch: expr) => {{
            let e = CharEscapeUnicode::new($ch);
            let output = e.as_str();
            let ans = $ch.escape_unicode().to_string();
            assert_eq!(output, ans);
        }};
    }

    test_char_escape_unicode!('\0');
    test_char_escape_unicode!('我');
    test_char_escape_unicode!('\u{10ffff}');
}

pub struct CharEscapeDebug {
    buf: [u8; 10],
    len: u8,
}

pub struct CharEscapeDebugArgs {
    pub escape_single_quote: bool,
    pub escape_double_quote: bool,
}

impl CharEscapeDebugArgs {
    #[cfg(test)]
    pub const ESCAPE_ALL: Self = Self {
        escape_single_quote: true,
        escape_double_quote: true,
    };
}

impl CharEscapeDebug {
    pub const fn new(ch: char, args: CharEscapeDebugArgs) -> Self {
        match ch {
            '\0' => Self::backslash_ascii(b'0'),
            '\t' => Self::backslash_ascii(b't'),
            '\r' => Self::backslash_ascii(b'r'),
            '\n' => Self::backslash_ascii(b'n'),
            '\\' => Self::backslash_ascii(b'\\'),
            '"' if args.escape_double_quote => Self::backslash_ascii(b'"'),
            '\'' if args.escape_single_quote => Self::backslash_ascii(b'\''),
            _ if is_printable(ch) => Self::printable(ch),
            _ => Self::unicode(ch),
        }
    }

    const fn printable(ch: char) -> Self {
        let e = CharEncodeUtf8::new(ch);
        Self {
            buf: [e.buf[0], e.buf[1], e.buf[2], e.buf[3], 0, 0, 0, 0, 0, 0],
            len: e.len,
        }
    }

    const fn backslash_ascii(ch: u8) -> Self {
        Self {
            buf: [b'\\', ch, 0, 0, 0, 0, 0, 0, 0, 0],
            len: 2,
        }
    }

    const fn unicode(ch: char) -> Self {
        let e = CharEscapeUnicode::new(ch);
        Self {
            buf: e.buf,
            len: e.len,
        }
    }

    pub const fn as_bytes(&self) -> &[u8] {
        crate::bytes::subslice(&self.buf, 0..self.len as usize)
    }

    #[cfg(test)]
    pub fn as_str(&self) -> &str {
        unsafe { core::str::from_utf8_unchecked(&self.buf[..self.len as usize]) }
    }

    // pub const fn to_str_buf<const N: usize>(&self) -> StrBuf<N> {
    //     let buf = crate::bytes::clone(self.as_bytes());
    //     unsafe { StrBuf::new_unchecked(buf) }
    // }
}

#[test]
fn test_char_escape_debug() {
    macro_rules! test_char_escape_debug {
        ($ch: expr) => {{
            let e = CharEscapeDebug::new($ch, CharEscapeDebugArgs::ESCAPE_ALL);
            let output = e.as_str();
            let ans = $ch.escape_debug().to_string();
            assert_eq!(output, ans);
        }};
    }

    for ch in '\0'..='\u{7f}' {
        test_char_escape_debug!(ch);
    }

    // test_char_escape_debug!('我');
    test_char_escape_debug!('\u{10ffff}');
}

pub const fn next_char(bytes: &[u8]) -> Option<(char, usize)> {
    /// Copied from [core::str::validations](https://github.com/rust-lang/rust/blob/e7958d35ca2c898a223efe402481e0ecb854310a/library/core/src/str/validations.rs#L7-L68)
    #[allow(clippy::many_single_char_names)]
    const fn next_code_point(bytes: &[u8]) -> Option<(u32, usize)> {
        const CONT_MASK: u8 = 0b0011_1111;

        const fn utf8_first_byte(byte: u8, width: u32) -> u32 {
            (byte & (0x7F >> width)) as u32
        }

        const fn utf8_acc_cont_byte(ch: u32, byte: u8) -> u32 {
            (ch << 6) | (byte & CONT_MASK) as u32
        }

        const fn unwrap_or_0(opt: Option<u8>) -> u8 {
            match opt {
                Some(byte) => byte,
                None => 0,
            }
        }

        let mut i = 0;

        macro_rules! next {
            () => {{
                if i < bytes.len() {
                    let x = Some(bytes[i]);
                    i += 1;
                    x
                } else {
                    None
                }
            }};
        }

        let x = match next!() {
            Some(x) => x,
            None => return None,
        };
        if x < 128 {
            return Some((x as u32, i));
        }

        let init = utf8_first_byte(x, 2);
        let y = unwrap_or_0(next!());
        let mut ch = utf8_acc_cont_byte(init, y);
        if x >= 0xE0 {
            let z = unwrap_or_0(next!());
            let y_z = utf8_acc_cont_byte((y & CONT_MASK) as u32, z);
            ch = init << 12 | y_z;
            if x >= 0xF0 {
                let w = unwrap_or_0(next!());
                ch = (init & 7) << 18 | utf8_acc_cont_byte(y_z, w);
            }
        }

        Some((ch, i))
    }

    match next_code_point(bytes) {
        Some((ch, count)) => Some((unsafe { crate::str::char_from_u32(ch) }, count)),
        None => None,
    }
}

pub const fn str_count_chars(s: &str) -> usize {
    let mut s = s.as_bytes();
    let mut ans = 0;
    while let Some((_, count)) = next_char(s) {
        s = crate::bytes::advance(s, count);
        ans += 1;
    }
    ans
}

pub const fn str_chars<const N: usize>(s: &str) -> [char; N] {
    let mut s = s.as_bytes();
    let mut buf: [char; N] = ['\0'; N];
    let mut pos = 0;
    while let Some((ch, count)) = next_char(s) {
        s = crate::bytes::advance(s, count);
        buf[pos] = ch;
        pos += 1;
    }
    constfn_assert!(pos == N);
    buf
}

#[test]
fn test_str_chars() {
    const X: &str = "唐可可";
    const OUTPUT_LEN: usize = str_count_chars(X);
    const OUTPUT_BUF: [char; OUTPUT_LEN] = str_chars::<OUTPUT_LEN>(X);
    let ans = X.chars().collect::<Vec<_>>();
    assert_eq!(OUTPUT_BUF, ans.as_slice());
}

// -----------------------------------------------------------------------------
// Copied from [core::str::validations](https://github.com/rust-lang/rust/blob/4ca19e09d302a4cbde14f9cb1bc109179dc824cd/library/core/src/str/validations.rs)

pub struct Utf8Error {
    pub valid_up_to: usize,
    pub error_len: Option<u8>,
}

// https://tools.ietf.org/html/rfc3629
const UTF8_CHAR_WIDTH: &[u8; 256] = &[
    // 1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 1
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 3
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 5
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 7
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 8
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B
    0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E
    4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F
];

/// Given a first byte, determines how many bytes are in this UTF-8 character.
#[must_use]
#[inline]
pub const fn utf8_char_width(b: u8) -> usize {
    UTF8_CHAR_WIDTH[b as usize] as usize
}

#[cfg(target_pointer_width = "16")]
const NONASCII_MASK: usize = 0x8080;

#[cfg(target_pointer_width = "32")]
const NONASCII_MASK: usize = 0x80808080;

#[cfg(target_pointer_width = "64")]
const NONASCII_MASK: usize = 0x8080808080808080;

/// Returns `true` if any byte in the word `x` is nonascii (>= 128).
#[inline]
const fn contains_nonascii(x: usize) -> bool {
    (x & NONASCII_MASK) != 0
}

pub const fn run_utf8_validation(v: &[u8]) -> Result<(), Utf8Error> {
    let mut index = 0;
    let len = v.len();

    let usize_bytes = mem::size_of::<usize>();
    let ascii_block_size = 2 * usize_bytes;
    let blocks_end = if len >= ascii_block_size {
        len - ascii_block_size + 1
    } else {
        0
    };
    // FIXME: waiting feature(const_align_offset)
    // <https://github.com/rust-lang/rust/issues/90962>
    // let align = v.as_ptr().align_offset(usize_bytes);
    let align = usize::MAX;

    while index < len {
        let old_offset = index;
        macro_rules! err {
            ($error_len: expr) => {
                return Err(Utf8Error {
                    valid_up_to: old_offset,
                    error_len: $error_len,
                })
            };
        }

        macro_rules! next {
            () => {{
                index += 1;
                // we needed data, but there was none: error!
                if index >= len {
                    err!(None)
                }
                v[index]
            }};
        }

        let first = v[index];
        if first >= 128 {
            let w = utf8_char_width(first);
            // 2-byte encoding is for codepoints  \u{0080} to  \u{07ff}
            //        first  C2 80        last DF BF
            // 3-byte encoding is for codepoints  \u{0800} to  \u{ffff}
            //        first  E0 A0 80     last EF BF BF
            //   excluding surrogates codepoints  \u{d800} to  \u{dfff}
            //               ED A0 80 to       ED BF BF
            // 4-byte encoding is for codepoints \u{1000}0 to \u{10ff}ff
            //        first  F0 90 80 80  last F4 8F BF BF
            //
            // Use the UTF-8 syntax from the RFC
            //
            // https://tools.ietf.org/html/rfc3629
            // UTF8-1      = %x00-7F
            // UTF8-2      = %xC2-DF UTF8-tail
            // UTF8-3      = %xE0 %xA0-BF UTF8-tail / %xE1-EC 2( UTF8-tail ) /
            //               %xED %x80-9F UTF8-tail / %xEE-EF 2( UTF8-tail )
            // UTF8-4      = %xF0 %x90-BF 2( UTF8-tail ) / %xF1-F3 3( UTF8-tail ) /
            //               %xF4 %x80-8F 2( UTF8-tail )
            match w {
                2 => {
                    if next!() as i8 >= -64 {
                        err!(Some(1))
                    }
                }
                3 => {
                    match (first, next!()) {
                        (0xE0, 0xA0..=0xBF)
                        | (0xE1..=0xEC, 0x80..=0xBF)
                        | (0xED, 0x80..=0x9F)
                        | (0xEE..=0xEF, 0x80..=0xBF) => {}
                        _ => err!(Some(1)),
                    }
                    if next!() as i8 >= -64 {
                        err!(Some(2))
                    }
                }
                4 => {
                    match (first, next!()) {
                        (0xF0, 0x90..=0xBF) | (0xF1..=0xF3, 0x80..=0xBF) | (0xF4, 0x80..=0x8F) => {}
                        _ => err!(Some(1)),
                    }
                    if next!() as i8 >= -64 {
                        err!(Some(2))
                    }
                    if next!() as i8 >= -64 {
                        err!(Some(3))
                    }
                }
                _ => err!(Some(1)),
            }
            index += 1;
        } else {
            // Ascii case, try to skip forward quickly.
            // When the pointer is aligned, read 2 words of data per iteration
            // until we find a word containing a non-ascii byte.
            if align != usize::MAX && align.wrapping_sub(index) % usize_bytes == 0 {
                let ptr = v.as_ptr();
                while index < blocks_end {
                    // SAFETY: since `align - index` and `ascii_block_size` are
                    // multiples of `usize_bytes`, `block = ptr.add(index)` is
                    // always aligned with a `usize` so it's safe to dereference
                    // both `block` and `block.offset(1)`.
                    unsafe {
                        let block = ptr.add(index) as *const usize;
                        // break if there is a nonascii byte
                        let zu = contains_nonascii(*block);
                        let zv = contains_nonascii(*block.offset(1));
                        if zu || zv {
                            break;
                        }
                    }
                    index += ascii_block_size;
                }
                // step from the point where the wordwise loop stopped
                while index < len && v[index] < 128 {
                    index += 1;
                }
            } else {
                index += 1;
            }
        }
    }

    Ok(())
}
