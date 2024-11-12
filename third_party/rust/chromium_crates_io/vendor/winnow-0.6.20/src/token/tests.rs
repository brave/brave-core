use super::*;

#[cfg(feature = "std")]
use proptest::prelude::*;

use crate::ascii::Caseless;
use crate::combinator::delimited;
use crate::error::ErrMode;
use crate::error::ErrorKind;
use crate::error::InputError;
use crate::error::Needed;
use crate::stream::AsChar;
use crate::token::literal;
use crate::unpeek;
use crate::IResult;
use crate::Parser;
use crate::Partial;

#[test]
fn complete_take_while_m_n_utf8_all_matching() {
    let result: IResult<&str, &str> =
        take_while(1..=4, |c: char| c.is_alphabetic()).parse_peek("øn");
    assert_eq!(result, Ok(("", "øn")));
}

#[test]
fn complete_take_while_m_n_utf8_all_matching_substring() {
    let result: IResult<&str, &str> = take_while(1, |c: char| c.is_alphabetic()).parse_peek("øn");
    assert_eq!(result, Ok(("n", "ø")));
}

#[cfg(feature = "std")]
fn model_complete_take_while_m_n(
    m: usize,
    n: usize,
    valid: usize,
    input: &str,
) -> IResult<&str, &str> {
    if n < m {
        Err(crate::error::ErrMode::from_error_kind(
            &input,
            crate::error::ErrorKind::Slice,
        ))
    } else if m <= valid {
        let offset = n.min(valid);
        Ok((&input[offset..], &input[0..offset]))
    } else {
        Err(crate::error::ErrMode::from_error_kind(
            &input,
            crate::error::ErrorKind::Slice,
        ))
    }
}

#[cfg(feature = "std")]
proptest! {
  #[test]
  #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
  fn complete_take_while_m_n_bounds(m in 0..20usize, n in 0..20usize, valid in 0..20usize, invalid in 0..20usize) {
      let input = format!("{:a<valid$}{:b<invalid$}", "", "", valid=valid, invalid=invalid);
      let expected = model_complete_take_while_m_n(m, n, valid, &input);
      if m <= n {
          let actual = take_while(m..=n, |c: char| c == 'a').parse_peek(input.as_str());
          assert_eq!(expected, actual);
      }
  }
}

#[test]
fn complete_take_until() {
    fn take_until_5_10(i: &str) -> IResult<&str, &str> {
        take_until(5..=8, "end").parse_peek(i)
    }
    assert_eq!(
        take_until_5_10("end"),
        Err(ErrMode::Backtrack(error_position!(
            &"end",
            ErrorKind::Slice
        )))
    );
    assert_eq!(
        take_until_5_10("1234end"),
        Err(ErrMode::Backtrack(error_position!(
            &"1234end",
            ErrorKind::Slice
        )))
    );
    assert_eq!(take_until_5_10("12345end"), Ok(("end", "12345")));
    assert_eq!(take_until_5_10("123456end"), Ok(("end", "123456")));
    assert_eq!(take_until_5_10("12345678end"), Ok(("end", "12345678")));
    assert_eq!(
        take_until_5_10("123456789end"),
        Err(ErrMode::Backtrack(error_position!(
            &"123456789end",
            ErrorKind::Slice
        )))
    );
}

#[test]
fn complete_take_until_empty() {
    fn take_until_empty(i: &str) -> IResult<&str, &str> {
        take_until(0, "").parse_peek(i)
    }
    assert_eq!(take_until_empty(""), Ok(("", "")));
    assert_eq!(take_until_empty("end"), Ok(("end", "")));
}

#[test]
fn complete_literal_case_insensitive() {
    fn caseless_bytes(i: &[u8]) -> IResult<&[u8], &[u8]> {
        literal(Caseless("ABcd")).parse_peek(i)
    }
    assert_eq!(
        caseless_bytes(&b"aBCdefgh"[..]),
        Ok((&b"efgh"[..], &b"aBCd"[..]))
    );
    assert_eq!(
        caseless_bytes(&b"abcdefgh"[..]),
        Ok((&b"efgh"[..], &b"abcd"[..]))
    );
    assert_eq!(
        caseless_bytes(&b"ABCDefgh"[..]),
        Ok((&b"efgh"[..], &b"ABCD"[..]))
    );
    assert_eq!(
        caseless_bytes(&b"ab"[..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"ab"[..],
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        caseless_bytes(&b"Hello"[..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"Hello"[..],
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        caseless_bytes(&b"Hel"[..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"Hel"[..],
            ErrorKind::Tag
        )))
    );

    fn caseless_str(i: &str) -> IResult<&str, &str> {
        literal(Caseless("ABcd")).parse_peek(i)
    }
    assert_eq!(caseless_str("aBCdefgh"), Ok(("efgh", "aBCd")));
    assert_eq!(caseless_str("abcdefgh"), Ok(("efgh", "abcd")));
    assert_eq!(caseless_str("ABCDefgh"), Ok(("efgh", "ABCD")));
    assert_eq!(
        caseless_str("ab"),
        Err(ErrMode::Backtrack(error_position!(&"ab", ErrorKind::Tag)))
    );
    assert_eq!(
        caseless_str("Hello"),
        Err(ErrMode::Backtrack(error_position!(
            &"Hello",
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        caseless_str("Hel"),
        Err(ErrMode::Backtrack(error_position!(&"Hel", ErrorKind::Tag)))
    );

    fn matches_kelvin(i: &str) -> IResult<&str, &str> {
        literal(Caseless("k")).parse_peek(i)
    }
    assert_eq!(
        matches_kelvin("K"),
        Err(ErrMode::Backtrack(error_position!(&"K", ErrorKind::Tag)))
    );

    fn is_kelvin(i: &str) -> IResult<&str, &str> {
        literal(Caseless("K")).parse_peek(i)
    }
    assert_eq!(
        is_kelvin("k"),
        Err(ErrMode::Backtrack(error_position!(&"k", ErrorKind::Tag)))
    );
}

#[test]
fn complete_literal_fixed_size_array() {
    fn test(i: &[u8]) -> IResult<&[u8], &[u8]> {
        literal([0x42]).parse_peek(i)
    }
    fn test2(i: &[u8]) -> IResult<&[u8], &[u8]> {
        literal(&[0x42]).parse_peek(i)
    }

    let input = &[0x42, 0x00][..];
    assert_eq!(test(input), Ok((&b"\x00"[..], &b"\x42"[..])));
    assert_eq!(test2(input), Ok((&b"\x00"[..], &b"\x42"[..])));
}

#[test]
fn complete_literal_char() {
    fn test(i: &[u8]) -> IResult<&[u8], &[u8]> {
        literal('B').parse_peek(i)
    }
    assert_eq!(test(&[0x42, 0x00][..]), Ok((&b"\x00"[..], &b"\x42"[..])));
    assert_eq!(
        test(&[b'A', b'\0'][..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"A\0"[..],
            ErrorKind::Tag
        )))
    );
}

#[test]
fn complete_literal_byte() {
    fn test(i: &[u8]) -> IResult<&[u8], &[u8]> {
        literal(b'B').parse_peek(i)
    }
    assert_eq!(test(&[0x42, 0x00][..]), Ok((&b"\x00"[..], &b"\x42"[..])));
    assert_eq!(
        test(&[b'A', b'\0'][..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"A\0"[..],
            ErrorKind::Tag
        )))
    );
}

#[test]
fn partial_any_str() {
    use super::any;
    assert_eq!(
        any::<_, InputError<Partial<&str>>>.parse_peek(Partial::new("Ә")),
        Ok((Partial::new(""), 'Ә'))
    );
}

#[test]
fn partial_one_of_test() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, u8> {
        one_of(['a', 'b']).parse_peek(i)
    }

    let a = &b"abcd"[..];
    assert_eq!(f(Partial::new(a)), Ok((Partial::new(&b"bcd"[..]), b'a')));

    let b = &b"cde"[..];
    assert_eq!(
        f(Partial::new(b)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(b),
            ErrorKind::Verify
        )))
    );

    fn utf8(i: Partial<&str>) -> IResult<Partial<&str>, char> {
        one_of(['+', '\u{FF0B}']).parse_peek(i)
    }

    assert!(utf8(Partial::new("+")).is_ok());
    assert!(utf8(Partial::new("\u{FF0B}")).is_ok());
}

#[test]
fn char_byteslice() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, char> {
        'c'.parse_peek(i)
    }

    let a = &b"abcd"[..];
    assert_eq!(
        f(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(a),
            ErrorKind::Tag
        )))
    );

    let b = &b"cde"[..];
    assert_eq!(f(Partial::new(b)), Ok((Partial::new(&b"de"[..]), 'c')));
}

#[test]
fn char_str() {
    fn f(i: Partial<&str>) -> IResult<Partial<&str>, char> {
        'c'.parse_peek(i)
    }

    let a = "abcd";
    assert_eq!(
        f(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(a),
            ErrorKind::Tag
        )))
    );

    let b = "cde";
    assert_eq!(f(Partial::new(b)), Ok((Partial::new("de"), 'c')));
}

#[test]
fn partial_none_of_test() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, u8> {
        none_of(['a', 'b']).parse_peek(i)
    }

    let a = &b"abcd"[..];
    assert_eq!(
        f(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(a),
            ErrorKind::Verify
        )))
    );

    let b = &b"cde"[..];
    assert_eq!(f(Partial::new(b)), Ok((Partial::new(&b"de"[..]), b'c')));
}

#[test]
fn partial_is_a() {
    fn a_or_b(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_while(1.., ['a', 'b']).parse_peek(i)
    }

    let a = Partial::new(&b"abcd"[..]);
    assert_eq!(a_or_b(a), Ok((Partial::new(&b"cd"[..]), &b"ab"[..])));

    let b = Partial::new(&b"bcde"[..]);
    assert_eq!(a_or_b(b), Ok((Partial::new(&b"cde"[..]), &b"b"[..])));

    let c = Partial::new(&b"cdef"[..]);
    assert_eq!(
        a_or_b(c),
        Err(ErrMode::Backtrack(error_position!(&c, ErrorKind::Slice)))
    );

    let d = Partial::new(&b"bacdef"[..]);
    assert_eq!(a_or_b(d), Ok((Partial::new(&b"cdef"[..]), &b"ba"[..])));
}

#[test]
fn partial_is_not() {
    fn a_or_b(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_till(1.., ['a', 'b']).parse_peek(i)
    }

    let a = Partial::new(&b"cdab"[..]);
    assert_eq!(a_or_b(a), Ok((Partial::new(&b"ab"[..]), &b"cd"[..])));

    let b = Partial::new(&b"cbde"[..]);
    assert_eq!(a_or_b(b), Ok((Partial::new(&b"bde"[..]), &b"c"[..])));

    let c = Partial::new(&b"abab"[..]);
    assert_eq!(
        a_or_b(c),
        Err(ErrMode::Backtrack(error_position!(&c, ErrorKind::Slice)))
    );

    let d = Partial::new(&b"cdefba"[..]);
    assert_eq!(a_or_b(d), Ok((Partial::new(&b"ba"[..]), &b"cdef"[..])));

    let e = Partial::new(&b"e"[..]);
    assert_eq!(a_or_b(e), Err(ErrMode::Incomplete(Needed::new(1))));
}

#[test]
fn partial_take_until_incomplete() {
    fn y(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_until(0.., "end").parse_peek(i)
    }
    assert_eq!(
        y(Partial::new(&b"nd"[..])),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
    assert_eq!(
        y(Partial::new(&b"123"[..])),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
    assert_eq!(
        y(Partial::new(&b"123en"[..])),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
}

#[test]
fn partial_take_until_incomplete_s() {
    fn ys(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_until(0.., "end").parse_peek(i)
    }
    assert_eq!(
        ys(Partial::new("123en")),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
}

#[test]
fn partial_take() {
    use crate::ascii::{
        alpha1 as alpha, alphanumeric1 as alphanumeric, digit1 as digit, hex_digit1 as hex_digit,
        multispace1 as multispace, oct_digit1 as oct_digit, space1 as space,
    };

    fn x(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        delimited("<!--", take(5_usize), "-->").take().parse_peek(i)
    }
    let r = x(Partial::new(&b"<!-- abc --> aaa"[..]));
    assert_eq!(r, Ok((Partial::new(&b" aaa"[..]), &b"<!-- abc -->"[..])));

    let semicolon = &b";"[..];

    fn ya(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        alpha.take().parse_peek(i)
    }
    let ra = ya(Partial::new(&b"abc;"[..]));
    assert_eq!(ra, Ok((Partial::new(semicolon), &b"abc"[..])));

    fn yd(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        digit.take().parse_peek(i)
    }
    let rd = yd(Partial::new(&b"123;"[..]));
    assert_eq!(rd, Ok((Partial::new(semicolon), &b"123"[..])));

    fn yhd(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        hex_digit.take().parse_peek(i)
    }
    let rhd = yhd(Partial::new(&b"123abcDEF;"[..]));
    assert_eq!(rhd, Ok((Partial::new(semicolon), &b"123abcDEF"[..])));

    fn yod(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        oct_digit.take().parse_peek(i)
    }
    let rod = yod(Partial::new(&b"1234567;"[..]));
    assert_eq!(rod, Ok((Partial::new(semicolon), &b"1234567"[..])));

    fn yan(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        alphanumeric.take().parse_peek(i)
    }
    let ran = yan(Partial::new(&b"123abc;"[..]));
    assert_eq!(ran, Ok((Partial::new(semicolon), &b"123abc"[..])));

    fn ys(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        space.take().parse_peek(i)
    }
    let rs = ys(Partial::new(&b" \t;"[..]));
    assert_eq!(rs, Ok((Partial::new(semicolon), &b" \t"[..])));

    fn yms(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        multispace.take().parse_peek(i)
    }
    let rms = yms(Partial::new(&b" \t\r\n;"[..]));
    assert_eq!(rms, Ok((Partial::new(semicolon), &b" \t\r\n"[..])));
}

#[test]
fn partial_take_while0() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_while(0.., AsChar::is_alpha).parse_peek(i)
    }
    let a = &b""[..];
    let b = &b"abcd"[..];
    let c = &b"abcd123"[..];
    let d = &b"123"[..];

    assert_eq!(f(Partial::new(a)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(f(Partial::new(b)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(f(Partial::new(c)), Ok((Partial::new(d), b)));
    assert_eq!(f(Partial::new(d)), Ok((Partial::new(d), a)));
}

#[test]
fn partial_take_while1() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_while(1.., AsChar::is_alpha).parse_peek(i)
    }
    let a = &b""[..];
    let b = &b"abcd"[..];
    let c = &b"abcd123"[..];
    let d = &b"123"[..];

    assert_eq!(f(Partial::new(a)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(f(Partial::new(b)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(f(Partial::new(c)), Ok((Partial::new(&b"123"[..]), b)));
    assert_eq!(
        f(Partial::new(d)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(d),
            ErrorKind::Slice
        )))
    );
}

#[test]
fn partial_take_while_m_n() {
    fn x(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_while(2..=4, AsChar::is_alpha).parse_peek(i)
    }
    let a = &b""[..];
    let b = &b"a"[..];
    let c = &b"abc"[..];
    let d = &b"abc123"[..];
    let e = &b"abcde"[..];
    let f = &b"123"[..];

    assert_eq!(x(Partial::new(a)), Err(ErrMode::Incomplete(Needed::new(2))));
    assert_eq!(x(Partial::new(b)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(x(Partial::new(c)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(x(Partial::new(d)), Ok((Partial::new(&b"123"[..]), c)));
    assert_eq!(
        x(Partial::new(e)),
        Ok((Partial::new(&b"e"[..]), &b"abcd"[..]))
    );
    assert_eq!(
        x(Partial::new(f)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(f),
            ErrorKind::Slice
        )))
    );
}

#[test]
fn partial_take_till0() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_till(0.., AsChar::is_alpha).parse_peek(i)
    }
    let a = &b""[..];
    let b = &b"abcd"[..];
    let c = &b"123abcd"[..];
    let d = &b"123"[..];

    assert_eq!(f(Partial::new(a)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(
        f(Partial::new(b)),
        Ok((Partial::new(&b"abcd"[..]), &b""[..]))
    );
    assert_eq!(
        f(Partial::new(c)),
        Ok((Partial::new(&b"abcd"[..]), &b"123"[..]))
    );
    assert_eq!(f(Partial::new(d)), Err(ErrMode::Incomplete(Needed::new(1))));
}

#[test]
fn partial_take_till1() {
    fn f(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_till(1.., AsChar::is_alpha).parse_peek(i)
    }
    let a = &b""[..];
    let b = &b"abcd"[..];
    let c = &b"123abcd"[..];
    let d = &b"123"[..];

    assert_eq!(f(Partial::new(a)), Err(ErrMode::Incomplete(Needed::new(1))));
    assert_eq!(
        f(Partial::new(b)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(b),
            ErrorKind::Slice
        )))
    );
    assert_eq!(
        f(Partial::new(c)),
        Ok((Partial::new(&b"abcd"[..]), &b"123"[..]))
    );
    assert_eq!(f(Partial::new(d)), Err(ErrMode::Incomplete(Needed::new(1))));
}

#[test]
fn partial_take_while_utf8() {
    fn f(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(0.., |c| c != '點').parse_peek(i)
    }

    assert_eq!(
        f(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        f(Partial::new("abcd")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(f(Partial::new("abcd點")), Ok((Partial::new("點"), "abcd")));
    assert_eq!(
        f(Partial::new("abcd點a")),
        Ok((Partial::new("點a"), "abcd"))
    );

    fn g(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(0.., |c| c == '點').parse_peek(i)
    }

    assert_eq!(
        g(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(g(Partial::new("點abcd")), Ok((Partial::new("abcd"), "點")));
    assert_eq!(
        g(Partial::new("點點點a")),
        Ok((Partial::new("a"), "點點點"))
    );
}

#[test]
fn partial_take_till0_utf8() {
    fn f(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_till(0.., |c| c == '點').parse_peek(i)
    }

    assert_eq!(
        f(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        f(Partial::new("abcd")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(f(Partial::new("abcd點")), Ok((Partial::new("點"), "abcd")));
    assert_eq!(
        f(Partial::new("abcd點a")),
        Ok((Partial::new("點a"), "abcd"))
    );

    fn g(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_till(0.., |c| c != '點').parse_peek(i)
    }

    assert_eq!(
        g(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(g(Partial::new("點abcd")), Ok((Partial::new("abcd"), "點")));
    assert_eq!(
        g(Partial::new("點點點a")),
        Ok((Partial::new("a"), "點點點"))
    );
}

#[test]
fn partial_take_utf8() {
    fn f(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take(3_usize).parse_peek(i)
    }

    assert_eq!(
        f(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
    assert_eq!(
        f(Partial::new("ab")),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
    assert_eq!(
        f(Partial::new("點")),
        Err(ErrMode::Incomplete(Needed::Unknown))
    );
    assert_eq!(f(Partial::new("ab點cd")), Ok((Partial::new("cd"), "ab點")));
    assert_eq!(f(Partial::new("a點bcd")), Ok((Partial::new("cd"), "a點b")));
    assert_eq!(f(Partial::new("a點b")), Ok((Partial::new(""), "a點b")));

    fn g(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(0.., |c| c == '點').parse_peek(i)
    }

    assert_eq!(
        g(Partial::new("")),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(g(Partial::new("點abcd")), Ok((Partial::new("abcd"), "點")));
    assert_eq!(
        g(Partial::new("點點點a")),
        Ok((Partial::new("a"), "點點點"))
    );
}

#[test]
fn partial_take_while_m_n_utf8_fixed() {
    fn parser(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(1, |c| c == 'A' || c == '😃').parse_peek(i)
    }
    assert_eq!(parser(Partial::new("A!")), Ok((Partial::new("!"), "A")));
    assert_eq!(parser(Partial::new("😃!")), Ok((Partial::new("!"), "😃")));
}

#[test]
fn partial_take_while_m_n_utf8_range() {
    fn parser(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(1..=2, |c| c == 'A' || c == '😃').parse_peek(i)
    }
    assert_eq!(parser(Partial::new("A!")), Ok((Partial::new("!"), "A")));
    assert_eq!(parser(Partial::new("😃!")), Ok((Partial::new("!"), "😃")));
}

#[test]
fn partial_take_while_m_n_utf8_full_match_fixed() {
    fn parser(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(1, |c: char| c.is_alphabetic()).parse_peek(i)
    }
    assert_eq!(parser(Partial::new("øn")), Ok((Partial::new("n"), "ø")));
}

#[test]
fn partial_take_while_m_n_utf8_full_match_range() {
    fn parser(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        take_while(1..=2, |c: char| c.is_alphabetic()).parse_peek(i)
    }
    assert_eq!(parser(Partial::new("øn")), Ok((Partial::new(""), "øn")));
}

#[test]
#[cfg(feature = "std")]
fn partial_take_take_while0() {
    fn x(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take_while(0.., AsChar::is_alphanum).parse_peek(i)
    }
    fn y(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        unpeek(x).take().parse_peek(i)
    }
    assert_eq!(
        x(Partial::new(&b"ab."[..])),
        Ok((Partial::new(&b"."[..]), &b"ab"[..]))
    );
    assert_eq!(
        y(Partial::new(&b"ab."[..])),
        Ok((Partial::new(&b"."[..]), &b"ab"[..]))
    );
}

#[test]
fn partial_literal_case_insensitive() {
    fn caseless_bytes(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        literal(Caseless("ABcd")).parse_peek(i)
    }
    assert_eq!(
        caseless_bytes(Partial::new(&b"aBCdefgh"[..])),
        Ok((Partial::new(&b"efgh"[..]), &b"aBCd"[..]))
    );
    assert_eq!(
        caseless_bytes(Partial::new(&b"abcdefgh"[..])),
        Ok((Partial::new(&b"efgh"[..]), &b"abcd"[..]))
    );
    assert_eq!(
        caseless_bytes(Partial::new(&b"ABCDefgh"[..])),
        Ok((Partial::new(&b"efgh"[..]), &b"ABCD"[..]))
    );
    assert_eq!(
        caseless_bytes(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        caseless_bytes(Partial::new(&b"Hello"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"Hello"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        caseless_bytes(Partial::new(&b"Hel"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"Hel"[..]),
            ErrorKind::Tag
        )))
    );

    fn caseless_str(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        literal(Caseless("ABcd")).parse_peek(i)
    }
    assert_eq!(
        caseless_str(Partial::new("aBCdefgh")),
        Ok((Partial::new("efgh"), "aBCd"))
    );
    assert_eq!(
        caseless_str(Partial::new("abcdefgh")),
        Ok((Partial::new("efgh"), "abcd"))
    );
    assert_eq!(
        caseless_str(Partial::new("ABCDefgh")),
        Ok((Partial::new("efgh"), "ABCD"))
    );
    assert_eq!(
        caseless_str(Partial::new("ab")),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        caseless_str(Partial::new("Hello")),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new("Hello"),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        caseless_str(Partial::new("Hel")),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new("Hel"),
            ErrorKind::Tag
        )))
    );

    fn matches_kelvin(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        literal(Caseless("k")).parse_peek(i)
    }
    assert_eq!(
        matches_kelvin(Partial::new("K")),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new("K"),
            ErrorKind::Tag
        )))
    );

    fn is_kelvin(i: Partial<&str>) -> IResult<Partial<&str>, &str> {
        literal(Caseless("K")).parse_peek(i)
    }
    assert_eq!(
        is_kelvin(Partial::new("k")),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new("k"),
            ErrorKind::Tag
        )))
    );
}

#[test]
fn partial_literal_fixed_size_array() {
    fn test(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        literal([0x42]).parse_peek(i)
    }
    fn test2(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        literal(&[0x42]).parse_peek(i)
    }
    let input = Partial::new(&[0x42, 0x00][..]);
    assert_eq!(test(input), Ok((Partial::new(&b"\x00"[..]), &b"\x42"[..])));
    assert_eq!(test2(input), Ok((Partial::new(&b"\x00"[..]), &b"\x42"[..])));
}
