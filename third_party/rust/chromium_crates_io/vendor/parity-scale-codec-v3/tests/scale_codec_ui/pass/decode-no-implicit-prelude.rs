#![no_implicit_prelude]

#[derive(::parity_scale_codec::Decode)]
#[codec(crate = ::parity_scale_codec)]
pub struct Struct {
    field_1: i8,
    field_2: i16,
    field_3: i32,
    field_4: i64,
}

#[derive(::parity_scale_codec::Decode)]
#[repr(transparent)]
struct Transparent {
    a: u8
}

#[derive(::parity_scale_codec::Decode)]
#[codec(crate = ::parity_scale_codec)]
pub enum Enum {
    Variant1,
    Variant2(i8, i16, i32, i64),
    Variant3 {
        field_1: i8,
        field_2: i16,
        field_3: i32,
        field_4: i64,
    }
}

fn main() {}
