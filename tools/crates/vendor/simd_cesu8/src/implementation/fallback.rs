#[must_use]
#[inline]
pub fn contains_null_or_utf8_4_byte_char_header(bytes: &[u8]) -> bool {
    for &byte in bytes {
        if byte == 0x00 || byte & 0b1111_1000 == 0b1111_0000 {
            return true;
        }
    }
    false
}

#[must_use]
#[inline]
pub fn contains_utf8_4_byte_char_header(bytes: &[u8]) -> bool {
    for &byte in bytes {
        if byte & 0b1111_1000 == 0b1111_0000 {
            return true;
        }
    }

    false
}
