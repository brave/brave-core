pub struct CharEncodeUtf16 {
    buf: [u16; 2],
}

impl CharEncodeUtf16 {
    /// Copied from [char::encode_utf16](https://github.com/rust-lang/rust/blob/0273e3bce7a0ce49e96a9662163e2380cb87e0be/library/core/src/char/methods.rs#L1647-L1682)
    pub const fn new(ch: char) -> Self {
        let mut code = ch as u32;
        let mut buf = [0; 2];
        if (code & 0xFFFF) == code {
            buf[0] = code as u16;
        } else {
            code -= 0x1_0000;
            buf[0] = 0xD800 | ((code >> 10) as u16);
            buf[1] = 0xDC00 | ((code as u16) & 0x3FF);
        }
        Self { buf }
    }

    pub const fn has_second(&self) -> bool {
        self.buf[1] != 0
    }

    pub const fn first(&self) -> u16 {
        self.buf[0]
    }
    pub const fn second(&self) -> u16 {
        self.buf[1]
    }
}

pub const fn str_len_utf16(s: &str) -> usize {
    let mut s = s.as_bytes();
    let mut ans = 0;
    while let Some((ch, count)) = crate::utf8::next_char(s) {
        s = crate::bytes::advance(s, count);
        ans += ch.len_utf16(); // const since 1.52
    }
    ans
}
