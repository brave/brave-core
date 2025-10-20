pub const fn num_to_hex_digit(x: u8) -> u8 {
    match x {
        0..=9 => b'0' + x,
        10..=15 => b'a' + (x - 10),
        _ => constfn_panic!("invalid hex number"),
    }
}

pub const fn num_from_dec_digit(d: u8) -> u8 {
    match d {
        b'0'..=b'9' => d - b'0',
        _ => constfn_panic!("invalid dec digit"),
    }
}
