use super::*;

macro_rules! write_to_le_bytes {
    ($($s:ident),*) => {
        $(/// Implementation detail
        pub fn $s(b: &$s) {
            write(&b.to_le_bytes())
        })*
    };
}

write_to_le_bytes!(u8, u16, u32, u64, u128, i8, i16, i32, i64, i128);

/// Implementation detail
pub fn usize(b: &usize) {
    write(&(*b as u32).to_le_bytes())
}

/// Implementation detail
pub fn isize(b: &isize) {
    write(&(*b as i32).to_le_bytes())
}
