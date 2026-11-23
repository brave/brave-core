#![cfg(feature = "unstable-test")]

// NOTE these tests should live in `defmt-macros` but the expansion of the macros defined there
// depend on `defmt` and `defmt` depends on `defmt-macros` -- the circular dependency may get in
// the way of `cargo test`

// NOTE string interning is mocked when testing so that it does not do real interning. Instead
// the "interner" always returns a **7-bit** `u8` value that's bumped on every interning operation.
//
// In practice, this means that the following operation:
// ```
// fn foo(f: &mut Formatter) {
//     write!(f, "Hello")
// }
// ```
// writes a *different* index on each call. With real interning this operation always writes the
// same index.
//
// `fetch_string_index` returns the current index of the mocked interner. Use this
// when writing the expected output of a unit test.
//
// ```
// let mut f = Internalexport::make_formatter();
// let index = defmt::export::fetch_string_index();
// foo(&mut f); // NOTE increases the interner index
// assert_eq!(fetch_bytes(), [index]);
//
// let mut f = Internalexport::make_formatter();
// foo(&mut f);
// assert_eq!(fetch_bytes(), [index.wrapping_add(1)]);
//                               ^^^^^^^^^^^^^^^ account for the previous `foo` call
// ```
//
// The mocked string index is thread local so you can run unit tests in parallel.
// `fetch_string_index` returns the thread-local interner index.
//
// Additional notes:
//
// - the mocked index is 7 bits so its LEB128 encoding is the input byte

use defmt::{export::fetch_string_index, write, Debug2Format, Display2Format, Format, Formatter};

// Increase the 7-bit mocked interned index
fn inc(index: u16, n: u16) -> u16 {
    index.wrapping_add(n)
}

fn write_format<T: Format + ?Sized>(val: &T) {
    defmt::export::istr(&T::_format_tag());
    val._format_data();
}

macro_rules! check {
    ([$($x:expr),* $(,)?]) => {
        {
            let mut v = Vec::<u8>::new();
            $(
                v.extend(&($x).to_le_bytes());
            )*
            assert_eq!(defmt::export::fetch_bytes(), v);
        }
    };
}

macro_rules! check_format {
    ($format:expr, [$($x:expr),* $(,)?]  $(,)?) => {
        {
            let mut v = Vec::<u8>::new();
            $(
                v.extend(&($x).to_le_bytes());
            )*
            write_format($format);
            assert_eq!(defmt::export::fetch_bytes(), v);
        }
    }
}

#[test]
fn write() {
    let index = fetch_string_index();
    let g = defmt::export::make_formatter();
    write!(g, "The answer is {=u8}", 42);
    check!([
        index, // "The answer is {=u8}",
        42u8,  // u8 value
    ]);

    let g = defmt::export::make_formatter();
    write!(g, "The answer is {=?}", 42u8);
    check!([
        inc(index, 1), // "The answer is {=?}"
        inc(index, 2), // "{=u8}" / impl Format for u8
        42u8,          // u8 value
    ]);
}

#[test]
fn bitfields_mixed() {
    let index = fetch_string_index();
    let g = defmt::export::make_formatter();

    write!(
        g,
        "bitfields {0=7..12}, {1=0..5}",
        0b1110_0101_1111_0000u16, 0b1111_0000u8
    );
    check!([
        index, // bitfields {0=7..12}, {1=0..5}",
        0b1111_0000u8,
        0b1110_0101u8, // u16
        0b1111_0000u8, // u8
    ]);
}

#[test]
fn bitfields_across_octets() {
    let index = fetch_string_index();
    let g = defmt::export::make_formatter();

    write!(g, "bitfields {0=0..7} {0=9..14}", 0b0110_0011_1101_0010u16);
    check!([
        index, // bitfields {0=0..7} {0=9..14}",
        0b1101_0010u8,
        0b0110_0011u8, // u16
    ]);
}

#[test]
fn bitfields_truncate_lower() {
    let index = fetch_string_index();
    let g = defmt::export::make_formatter();

    write!(
        g,
        "bitfields {0=9..14}",
        0b0000_0000_0000_1111_0110_0011_1101_0010u32
    );
    check!([
        index,         // bitfields {0=9..14}",
        0b0110_0011u8, // the first octet should have been truncated away
    ]);
}

#[test]
fn bitfields_assert_range_exclusive() {
    let index = fetch_string_index();
    let g = defmt::export::make_formatter();

    write!(g, "bitfields {0=6..8}", 0b1010_0101u8,);
    check!([
        index, // "bitfields {0=6..8}"
        0b1010_0101u8
    ]);
}

#[test]
fn debug_attr_struct() {
    #[derive(Debug)]
    struct DebugOnly {
        _a: i32,
    }

    #[derive(Format)]
    struct X {
        y: bool,
        #[defmt(Debug2Format)]
        d: DebugOnly,
    }

    let index = fetch_string_index();
    check_format!(
        &X {
            y: false,
            d: DebugOnly { _a: 3 }
        },
        [
            index,         // "X {{ y: {=bool}, d: {=?} }}"
            0b0u8,         // y
            inc(index, 1), // DebugOnly's format string
            b'D',          // Text of the Debug output
            b'e',
            b'b',
            b'u',
            b'g',
            b'O',
            b'n',
            b'l',
            b'y',
            b' ',
            b'{',
            b' ',
            b'_',
            b'a',
            b':',
            b' ',
            b'3',
            b' ',
            b'}',
            0xffu8
        ],
    )
}

#[test]
fn display_attr_enum() {
    use std::fmt;
    struct DisplayOnly {}

    impl fmt::Display for DisplayOnly {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            f.write_str("Display")
        }
    }

    #[derive(Format)]
    enum X {
        #[allow(dead_code)]
        Bool(bool),
        Display(#[defmt(Display2Format)] DisplayOnly),
    }

    let index = fetch_string_index();
    check_format!(
        &X::Display(DisplayOnly {}),
        [
            index,         // "Bool({=bool})|Display({=?})"
            0b1u8,         // Variant: Display
            inc(index, 1), // DisplayOnly's format string
            b'D',          // Text of the Display output
            b'i',
            b's',
            b'p',
            b'l',
            b'a',
            b'y',
            0xffu8
        ],
    )
}

#[test]
fn boolean_struct() {
    #[derive(Format)]
    struct X {
        y: bool,
        z: bool,
    }

    let index = fetch_string_index();
    check_format!(
        &X { y: false, z: true },
        [
            index, // "X {{ y: {=bool}, z: {=bool} }}"
            0b0u8, // y
            0b1u8, // z
        ],
    )
}

#[test]
fn single_struct() {
    #[derive(Format)]
    struct X {
        y: u8,
        z: u16,
    }

    let index = fetch_string_index();
    check_format!(
        &X { y: 1, z: 2 },
        [
            index, // "X {{ y: {=u8}, z: {=u16} }}"
            1u8,   // x
            2u8,   // y.low
            0u8,   // y.high
        ],
    )
}

#[test]
fn single_struct_manual() {
    // Above `#[derive]`d impl should be equivalent to this:
    struct X {
        y: u8,
        z: u16,
    }

    impl Format for X {
        fn format(&self, f: Formatter) {
            defmt::write!(f, "X {{ y: {=u8}, z: {=u16} }}", self.y, self.z)
        }
    }

    let index = fetch_string_index();
    check_format!(
        &X { y: 1, z: 2 },
        [
            index,         // "{=__internal_FormatSequence}"
            inc(index, 1), // "X {{ y: {=u8}, z: {=u16} }}"
            1u8,           // y
            2u16,          // z
            0u16,          // terminator
        ],
    )
}

#[test]
fn single_struct_manual_multiwrite() {
    // Above `#[derive]`d impl should be equivalent to this:
    struct X {
        y: u8,
        z: u16,
    }

    impl Format for X {
        fn format(&self, f: Formatter) {
            defmt::write!(f, "y={=u8}", self.y);
            defmt::write!(f, "z={=u16}", self.z);
        }
    }

    let index = fetch_string_index();
    check_format!(
        &X { y: 1, z: 2 },
        [
            index,         // "{=__internal_FormatSequence}"
            inc(index, 1), // "y={=u8}"
            1u8,           // y
            inc(index, 2), // "z={=u16}"
            2u16,          // z
            0u16,          // terminator
        ],
    )
}

#[test]
fn slice_struct_manual_multiwrite() {
    // Above `#[derive]`d impl should be equivalent to this:
    struct X {
        y: u8,
        z: u16,
    }

    impl Format for X {
        fn format(&self, f: Formatter) {
            defmt::write!(f, "y={=u8}", self.y);
            defmt::write!(f, "z={=u16}", self.z);
        }
    }

    let index = fetch_string_index();
    check_format!(
        &[X { y: 1, z: 2 }, X { y: 3, z: 4 }][..],
        [
            index,         // "{=[?]}"
            2u32,          // len
            inc(index, 1), // "{=__internal_FormatSequence}"
            // first element
            inc(index, 2), // "y={=u8}"
            1u8,           // y
            inc(index, 3), // "z={=u16}"
            2u16,          // z
            0u16,          // terminator
            // second element
            inc(index, 4), // "y={=u8}"
            3u8,           // y
            inc(index, 5), // "z={=u16}"
            4u16,          // z
            0u16,          // terminator
        ],
    )
}

#[test]
fn nested_struct() {
    #[derive(Format)]
    struct X {
        y: Y,
    }

    #[derive(Format)]
    struct Y {
        z: u8,
    }

    let val = 42u8;
    let index = fetch_string_index();
    check_format!(
        &X { y: Y { z: val } },
        [
            index,         // "X {{ y: {=?} }}"
            inc(index, 1), // "Y {{ z: {=u8} }}"
            val,
        ],
    );
}

#[test]
fn tuple_struct() {
    #[derive(Format)]
    struct Struct(u8, u16);

    let index = fetch_string_index();
    check_format!(
        &Struct(0x1f, 0xaaaa),
        [
            index,     // "Struct({=u8}, {=u16})"
            0x1fu8,    // u8
            0xaaaau16, // u16
        ],
    );
}

#[test]
fn c_like_enum() {
    #[derive(Format)]
    #[allow(dead_code)]
    enum Enum {
        A,
        B,
        C,
    }

    let index = fetch_string_index();
    check_format!(
        &Enum::A,
        [
            index, //
            0u8,   // `Enum::A`
        ],
    );
    let index = fetch_string_index();
    check_format!(
        &Enum::B,
        [
            index, //
            1u8,   // `Enum::B`
        ],
    );
}

#[test]
#[allow(dead_code)]
fn uninhabited_enum() {
    #[derive(Format)]
    enum Void {}
}

/// Tests that univariant enums do not encode the discriminant (since the variant in use is always
/// the same).
#[test]
fn univariant_enum() {
    #[derive(Format)]
    enum NoData {
        Variant,
    }

    let index = fetch_string_index();
    check_format!(
        &NoData::Variant,
        [
            index, //
        ],
    );

    #[derive(Format)]
    enum Data {
        Variant(u8, u16),
    }

    let index = fetch_string_index();
    check_format!(
        &Data::Variant(0x1f, 0xaaaa),
        [
            index,     //
            0x1fu8,    // u8
            0xaaaau16, // u16
        ],
    );
}

#[test]
fn nested_enum() {
    #[derive(Format)]
    #[allow(dead_code)]
    enum CLike {
        A,
        B,
        C,
    }

    #[derive(Format)]
    enum Inner {
        A(CLike, u8),
        _B,
    }

    #[derive(Format)]
    enum Outer {
        Variant1 { pre: u8, inner: Inner, post: u8 },
        Variant2,
        Variant3(Inner),
    }

    let index = fetch_string_index();
    check_format!(
        &Outer::Variant1 {
            pre: 0xEE,
            inner: Inner::A(CLike::B, 0x07),
            post: 0xAB,
        },
        [
            index,         //
            0u8,           // `Outer::Variant1`
            0xEEu8,        // u8 pre
            inc(index, 1), // `Inner`'s formatting string
            0u8,           // `Inner::A`
            inc(index, 2), // `CLike`'s formatting string
            1u8,           // `CLike::B`
            0x07u8,        // u8
            0xABu8,        // u8 post
        ],
    );

    let index = fetch_string_index();
    check_format!(
        &Outer::Variant2,
        [
            index, //
            1u8,   // `Outer::Variant2`
        ],
    );

    let index = fetch_string_index();
    check_format!(
        &Outer::Variant3(Inner::A(CLike::B, 0x07)),
        [
            index,         //
            2u8,           // `Outer::Variant3`
            inc(index, 1), // `Inner`'s formatting string
            0u8,           // `Inner::A`
            inc(index, 2), // `CLike`'s formatting string
            1u8,           // `CLike::B`
            0x07u8,        // u8
        ],
    );
}

#[test]
fn slice() {
    let index = fetch_string_index();
    let val: &[u8] = &[23u8, 42u8];
    check_format!(
        val,
        [
            index,            // "{=[?]}"
            val.len() as u32, // length
            inc(index, 1),    // "{=u8}"
            23u8,             // val[0]
            42u8,             // val[1]
        ],
    )
}

#[test]
fn slice_of_usize() {
    let index = fetch_string_index();
    let val: &[usize] = &[23usize, 42];
    check_format!(
        val,
        [
            index,            // "{=[?]}"
            val.len() as u32, // length
            inc(index, 1),    // "{=usize}"
            23u32,            // val[0]
            42u32,            // val[1]
        ],
    )
}

#[test]
fn slice_of_bools() {
    let index = fetch_string_index();
    let val: &[bool] = &[true, true, false];
    check_format!(
        val,
        [
            index,            // "{=[?]}"
            val.len() as u32, // length
            inc(index, 1),    // "{=bool}"
            0b1u8,
            0b1u8,
            0b0u8,
        ],
    )
}

#[test]
fn format_primitives() {
    let index = fetch_string_index();
    check_format!(
        &42u8,
        [
            index, // "{=u8}"
            42u8,
        ],
    );
    check_format!(
        &42u16,
        [
            inc(index, 1), // "{=u16}"
            42u16,
        ],
    );
    check_format!(
        &513u16,
        [
            inc(index, 2), // "{=u16}"
            513u16,
        ],
    );

    check_format!(
        &42u32,
        [
            inc(index, 3), // "{=u32}"
            42u32,
        ],
    );
    check_format!(
        &513u32,
        [
            inc(index, 4), // "{=u32}"
            513u32,
        ],
    );

    check_format!(
        &5.13f32,
        [
            inc(index, 5), // "{=f32}"
            246u8,
            40u8,
            164u8,
            64u8,
        ],
    );

    check_format!(
        &42i8,
        [
            inc(index, 6), // "{=i8}"
            42u8,
        ],
    );
    check_format!(
        &-42i8,
        [
            inc(index, 7), // "{=i8}"
            -42i8 as u8,
        ],
    );

    check_format!(
        &None::<u8>,
        [
            inc(index, 8), // "<option-format-string>"
            0u8,           // None discriminant
        ],
    );

    check_format!(
        &Some(42u8),
        [
            inc(index, 9),  // "<option-format-string>"
            1u8,            // Some discriminant
            inc(index, 10), // "{=u8}"
            42u8,           // Some.0 field
        ],
    );

    check_format!(&-1isize, [inc(index, 11), -1i32]);
    check_format!(&-128isize, [inc(index, 12), -128i32]);

    check_format!(
        &true,
        [
            inc(index, 13), // "{=bool}"
            0b1u8,
        ],
    );

    check_format!(
        &513u64,
        [
            inc(index, 14), // "{=u64}"
            513u64,
        ],
    );

    check_format!(
        &-2i64,
        [
            inc(index, 15), // "{=i64}"
            -2i64,
        ],
    );

    check_format!(
        &'a',
        [
            inc(index, 16), // "{=char}"
            0x61u8,
            0x00u8,
            0x00u8,
            0x00u8,
        ],
    );
}

#[test]
fn istr() {
    let index = fetch_string_index();
    let interned = defmt::intern!("interned string contents");
    check_format!(
        &interned,
        [
            inc(index, 1), // "{=istr}"
            index,
        ],
    );
}

#[test]
fn format_arrays() {
    let index = fetch_string_index();
    let array: [u16; 0] = [];
    check_format!(
        &array,
        [
            index,         // "{=[?;0]}"
            inc(index, 1), // "{=u16}"
        ]
    );

    let index = fetch_string_index();
    let array: [u16; 3] = [1, 256, 257];
    check_format!(
        &array,
        [
            index,         // "{=[?;3]}"
            inc(index, 1), // "{=u16}"
            1u16,          // [0]
            256u16,        // [1]
            257u16,        // [2]
        ],
    );
}

#[test]
fn format_slice_of_primitives() {
    let index = fetch_string_index();
    let slice: &[u16] = &[1, 256, 257];
    check_format!(
        slice,
        [
            index,              // "{=[?]}"
            slice.len() as u32, //
            inc(index, 1),      // "{=u16}"
            1u16,               // [0]
            256u16,             // [1]
            257u16,             // [2]
        ],
    );
}

#[test]
fn format_slice_of_structs() {
    #[derive(Format)]
    struct X {
        y: Y,
    }

    #[derive(Format)]
    struct Y {
        z: u8,
    }

    let index = fetch_string_index();
    let slice: &[_] = &[X { y: Y { z: 42 } }, X { y: Y { z: 24 } }];
    check_format!(
        slice,
        [
            index,              // "{=[?]}"
            slice.len() as u32, //
            // first element
            inc(index, 1), // "X {{ y: {=?} }}"
            inc(index, 2), // "Y {{ z: {=u8} }}"
            42u8,          // [0].y.z
            // second element: no outer tag
            inc(index, 3), // "Y {{ z: {=u8} }}"
            24u8,          // [1].y.z
        ],
    );
}

#[test]
fn format_slice_of_slices() {
    let index = fetch_string_index();
    let slice: &[&[u16]] = &[&[256, 257], &[258, 259, 260]];
    check_format!(
        slice,
        [
            index,              // "{=[?]}"
            slice.len() as u32, //
            inc(index, 1),      // "{=[?]}"
            // first slice
            slice[0].len() as u32,
            inc(index, 2), // "{=u16}"
            256u16,        // [0][0]
            257u16,        // [0][1]
            // second slice
            slice[1].len() as u32,
            inc(index, 3), // "{=u16}"
            258u16,        // [1][0]
            259u16,        // [1][1]
            260u16,        // [1][2]
        ],
    );
}

#[test]
fn format_slice_enum_slice() {
    let index = fetch_string_index();
    let slice: &[Option<&[u8]>] = &[None, Some(&[42, 43])];
    check_format!(
        slice,
        [
            index,              // "{=[?]}"
            slice.len() as u32, //
            // first optional slice
            inc(index, 1), // "None|Some({=?})"
            0u8,           // discriminant
            // second optional slice
            // omitted: "None|Some({=?})" index
            1u8,           // discriminant
            inc(index, 2), // "{=[?]}" (the ? behind "Some({=?})")
            2u32,          // length of second optional slice
            inc(index, 3), // "{=u8}" (the ? behind "{=[?]}")
            42u8,
            // omitted: "{=u8}" index
            43u8,
        ],
    );
}

#[test]
fn format_slice_enum_generic_struct() {
    #[derive(Format)]
    struct S<T> {
        x: u8,
        y: T,
    }

    let index = fetch_string_index();
    let slice: &[Option<S<u8>>] = &[None, Some(S { x: 42, y: 43 })];
    check_format!(
        slice,
        [
            index,              // "{=[?]}"
            slice.len() as u32, //
            // first optional element
            inc(index, 1), // "None|Some({=?})"
            0u8,           // discriminant
            // second optional element
            // omitted: "None|Some({=?})" index
            1u8,           // discriminant
            inc(index, 2), // "S {{ x: {=u8}, y: {=?} }}" (the ? behind "Some({=?})")
            42u8,          // S.x
            inc(index, 3), // "{=u8}" (the ? behind S.y)
            43u8,          // S. y
        ],
    );
}

#[test]
fn derive_with_bounds() {
    #[derive(Format)]
    struct S<T: Copy> {
        val: T,
    }

    #[derive(Format)]
    struct S2<'a: 'b, 'b> {
        a: &'a u8,
        b: &'b u8,
    }

    let index = fetch_string_index();
    check_format!(
        &S { val: 0 },
        [
            index,         // "S {{ val: {=?} }}"
            inc(index, 1), // "{=i32}"
            0u32,
        ],
    );

    let index = fetch_string_index();
    check_format!(
        &S2 { a: &1, b: &2 },
        [
            index, // "S2 { a: {=u8}, b: {=u8} }}"
            1u8, 2u8,
        ],
    );
}

#[test]
fn format_bools() {
    #[derive(Format)]
    struct A(bool);

    #[derive(Format)]
    struct B(bool);

    let index = fetch_string_index();
    check_format!(
        &(A(true), B(true)),
        [
            index,         // "({=?}, {=?})"
            inc(index, 1), // "A({=bool})"
            0b1u8,         // A
            inc(index, 2), // "B({=bool})"
            0b1u8,         // B
        ],
    );
}

#[test]
fn enum_variants() {
    #[allow(dead_code)]
    #[derive(Format)]
    enum EnumSmall {
        A0,
        A1,
        A2,
    }

    #[rustfmt::skip]
    #[allow(dead_code)]
    #[derive(Format)]
    enum EnumLarge {
        A000, A001, A002, A003, A004, A005, A006, A007, A008, A009, A010, A011, A012, A013, A014, A015, A016, A017,
        A018, A019, A020, A021, A022, A023, A024, A025, A026, A027, A028, A029, A030, A031, A032, A033, A034, A035,
        A036, A037, A038, A039, A040, A041, A042, A043, A044, A045, A046, A047, A048, A049, A050, A051, A052, A053,
        A054, A055, A056, A057, A058, A059, A060, A061, A062, A063, A064, A065, A066, A067, A068, A069, A070, A071,
        A072, A073, A074, A075, A076, A077, A078, A079, A080, A081, A082, A083, A084, A085, A086, A087, A088, A089,
        A090, A091, A092, A093, A094, A095, A096, A097, A098, A099, A100, A101, A102, A103, A104, A105, A106, A107,
        A108, A109, A110, A111, A112, A113, A114, A115, A116, A117, A118, A119, A120, A121, A122, A123, A124, A125,
        A126, A127, A128, A129, A130, A131, A132, A133, A134, A135, A136, A137, A138, A139, A140, A141, A142, A143,
        A144, A145, A146, A147, A148, A149, A150, A151, A152, A153, A154, A155, A156, A157, A158, A159, A160, A161,
        A162, A163, A164, A165, A166, A167, A168, A169, A170, A171, A172, A173, A174, A175, A176, A177, A178, A179,
        A180, A181, A182, A183, A184, A185, A186, A187, A188, A189, A190, A191, A192, A193, A194, A195, A196, A197,
        A198, A199, A200, A201, A202, A203, A204, A205, A206, A207, A208, A209, A210, A211, A212, A213, A214, A215,
        A216, A217, A218, A219, A220, A221, A222, A223, A224, A225, A226, A227, A228, A229, A230, A231, A232, A233,
        A234, A235, A236, A237, A238, A239, A240, A241, A242, A243, A244, A245, A246, A247, A248, A249, A250, A251,
        A252, A253, A254, A255, A256, A257, A258, A259, A260, A261, A262, A263, A264, A265, A266, A267, A268, A269,
    }

    let e = EnumSmall::A2;

    let index = fetch_string_index();
    check_format!(&e, [index, 2u8]);

    let e = EnumLarge::A002;
    let index = fetch_string_index();
    check_format!(&e, [index, 2u16]);

    let e = EnumLarge::A269;
    let index = fetch_string_index();
    check_format!(&e, [index, 269u16]);
}

#[test]
fn derive_str() {
    #[derive(Format)]
    struct S {
        x: &'static str,
    }

    let s = S { x: "hi" };

    let index = fetch_string_index();
    check_format!(
        &s,
        [
            index, // "S {{ s: {:str} }}" (NOTE: `s` field is not {:?})
            // so no extra format string index here
            2u32,  // s.x.len()
            104u8, // b'h'
            105u8, // b'i'
        ],
    );
}

#[test]
fn core_fmt_adapters() {
    let index = fetch_string_index();
    check_format!(&Debug2Format(&123u8), [index, b'1', b'2', b'3', 0xffu8]);
    let index = fetch_string_index();
    check_format!(&Display2Format(&123u8), [index, b'1', b'2', b'3', 0xffu8]);
}
