use super::*;

use crate::ascii::digit1 as digit;
use crate::binary::u16;
use crate::binary::u8;
use crate::binary::Endianness;
use crate::error::ErrMode;
use crate::error::ErrorKind;
use crate::error::InputError;
use crate::error::Needed;
use crate::error::ParserError;
use crate::stream::Stream;
use crate::token::take;
use crate::unpeek;
use crate::IResult;
use crate::PResult;
use crate::Parser;
use crate::Partial;

#[cfg(feature = "alloc")]
use crate::lib::std::vec::Vec;

macro_rules! assert_parse(
  ($left: expr, $right: expr) => {
    let res: $crate::IResult<_, _, InputError<_>> = $left;
    assert_eq!(res, $right);
  };
);

#[test]
fn eof_on_slices() {
    let not_over: &[u8] = &b"Hello, world!"[..];
    let is_over: &[u8] = &b""[..];

    let res_not_over = eof.parse_peek(not_over);
    assert_parse!(
        res_not_over,
        Err(ErrMode::Backtrack(error_position!(
            &not_over,
            ErrorKind::Eof
        )))
    );

    let res_over = eof.parse_peek(is_over);
    assert_parse!(res_over, Ok((is_over, is_over)));
}

#[test]
fn eof_on_strs() {
    let not_over: &str = "Hello, world!";
    let is_over: &str = "";

    let res_not_over = eof.parse_peek(not_over);
    assert_parse!(
        res_not_over,
        Err(ErrMode::Backtrack(error_position!(
            &not_over,
            ErrorKind::Eof
        )))
    );

    let res_over = eof.parse_peek(is_over);
    assert_parse!(res_over, Ok((is_over, is_over)));
}

#[test]
fn rest_on_slices() {
    let input: &[u8] = &b"Hello, world!"[..];
    let empty: &[u8] = &b""[..];
    assert_parse!(rest.parse_peek(input), Ok((empty, input)));
}

#[test]
fn rest_on_strs() {
    let input: &str = "Hello, world!";
    let empty: &str = "";
    assert_parse!(rest.parse_peek(input), Ok((empty, input)));
}

#[test]
fn rest_len_on_slices() {
    let input: &[u8] = &b"Hello, world!"[..];
    assert_parse!(rest_len.parse_peek(input), Ok((input, input.len())));
}

use crate::lib::std::convert::From;
impl From<u32> for CustomError {
    fn from(_: u32) -> Self {
        CustomError
    }
}

impl<I> ParserError<I> for CustomError {
    fn from_error_kind(_: &I, _: ErrorKind) -> Self {
        CustomError
    }

    fn append(self, _: &I, _: ErrorKind) -> Self {
        CustomError
    }
}

struct CustomError;
#[allow(dead_code)]
fn custom_error(input: &[u8]) -> IResult<&[u8], &[u8], CustomError> {
    //fix_error!(input, CustomError<_>, alphanumeric)
    crate::ascii::alphanumeric1.parse_peek(input)
}

#[test]
fn test_parser_flat_map() {
    let input: &[u8] = &[3, 100, 101, 102, 103, 104][..];
    assert_parse!(
        u8.flat_map(take).parse_peek(input),
        Ok((&[103, 104][..], &[100, 101, 102][..]))
    );
}

#[allow(dead_code)]
fn test_closure_compiles_195(input: &[u8]) -> IResult<&[u8], ()> {
    u8.flat_map(|num| repeat(num as usize, u16(Endianness::Big)))
        .parse_peek(input)
}

#[test]
fn test_parser_verify_map() {
    let input: &[u8] = &[50][..];
    assert_parse!(
        u8.verify_map(|u| if u < 20 { Some(u) } else { None })
            .parse_peek(input),
        Err(ErrMode::Backtrack(InputError::new(
            &[50][..],
            ErrorKind::Verify
        )))
    );
    assert_parse!(
        u8.verify_map(|u| if u > 20 { Some(u) } else { None })
            .parse_peek(input),
        Ok((&[][..], 50))
    );
}

#[test]
fn test_parser_map_parser() {
    let input: &[u8] = &[100, 101, 102, 103, 104][..];
    assert_parse!(
        take(4usize).and_then(take(2usize)).parse_peek(input),
        Ok((&[104][..], &[100, 101][..]))
    );
}

#[test]
#[cfg(feature = "std")]
fn test_parser_into() {
    use crate::error::InputError;
    use crate::token::take;

    let mut parser = take::<_, _, InputError<_>>(3u8).output_into();
    let result: IResult<&[u8], Vec<u8>> = parser.parse_peek(&b"abcdefg"[..]);

    assert_eq!(result, Ok((&b"defg"[..], vec![97, 98, 99])));
}

#[test]
fn opt_test() {
    fn opt_abcd(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Option<&[u8]>> {
        opt("abcd").parse_peek(i)
    }

    let a = &b"abcdef"[..];
    let b = &b"bcdefg"[..];
    let c = &b"ab"[..];
    assert_eq!(
        opt_abcd(Partial::new(a)),
        Ok((Partial::new(&b"ef"[..]), Some(&b"abcd"[..])))
    );
    assert_eq!(
        opt_abcd(Partial::new(b)),
        Ok((Partial::new(&b"bcdefg"[..]), None))
    );
    assert_eq!(
        opt_abcd(Partial::new(c)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
fn peek_test() {
    fn peek_tag(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        peek("abcd").parse_peek(i)
    }

    assert_eq!(
        peek_tag(Partial::new(&b"abcdef"[..])),
        Ok((Partial::new(&b"abcdef"[..]), &b"abcd"[..]))
    );
    assert_eq!(
        peek_tag(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        peek_tag(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
}

#[test]
fn not_test() {
    fn not_aaa(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, ()> {
        not("aaa").parse_peek(i)
    }

    assert_eq!(
        not_aaa(Partial::new(&b"aaa"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"aaa"[..]),
            ErrorKind::Not
        )))
    );
    assert_eq!(
        not_aaa(Partial::new(&b"aa"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        not_aaa(Partial::new(&b"abcd"[..])),
        Ok((Partial::new(&b"abcd"[..]), ()))
    );
}

#[test]
fn test_parser_verify() {
    use crate::token::take;

    fn test(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        take(5u8)
            .verify(|slice: &[u8]| slice[0] == b'a')
            .parse_peek(i)
    }
    assert_eq!(
        test(Partial::new(&b"bcd"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        test(Partial::new(&b"bcdefg"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"bcdefg"[..]),
            ErrorKind::Verify
        )))
    );
    assert_eq!(
        test(Partial::new(&b"abcdefg"[..])),
        Ok((Partial::new(&b"fg"[..]), &b"abcde"[..]))
    );
}

#[test]
#[allow(unused)]
fn test_parser_verify_ref() {
    use crate::token::take;

    let mut parser1 = take(3u8).verify(|s: &[u8]| s == &b"abc"[..]);

    assert_eq!(
        parser1.parse_peek(&b"abcd"[..]),
        Ok((&b"d"[..], &b"abc"[..]))
    );
    assert_eq!(
        parser1.parse_peek(&b"defg"[..]),
        Err(ErrMode::Backtrack(InputError::new(
            &b"defg"[..],
            ErrorKind::Verify
        )))
    );

    fn parser2(i: &[u8]) -> IResult<&[u8], u32> {
        crate::binary::be_u32
            .verify(|val: &u32| *val < 3)
            .parse_peek(i)
    }
}

#[test]
#[cfg(feature = "alloc")]
fn test_parser_verify_alloc() {
    use crate::token::take;
    let mut parser1 = take(3u8)
        .map(|s: &[u8]| s.to_vec())
        .verify(|s: &[u8]| s == &b"abc"[..]);

    assert_eq!(
        parser1.parse_peek(&b"abcd"[..]),
        Ok((&b"d"[..], b"abc".to_vec()))
    );
    assert_eq!(
        parser1.parse_peek(&b"defg"[..]),
        Err(ErrMode::Backtrack(InputError::new(
            &b"defg"[..],
            ErrorKind::Verify
        )))
    );
}

#[test]
fn fail_test() {
    let a = "string";
    let b = "another string";

    assert_eq!(
        fail::<_, &str, _>.parse_peek(a),
        Err(ErrMode::Backtrack(InputError::new(a, ErrorKind::Fail)))
    );
    assert_eq!(
        fail::<_, &str, _>.parse_peek(b),
        Err(ErrMode::Backtrack(InputError::new(b, ErrorKind::Fail)))
    );
}

#[test]
fn complete() {
    fn err_test(i: &[u8]) -> IResult<&[u8], &[u8]> {
        let (i, _) = "ijkl".parse_peek(i)?;
        "mnop".parse_peek(i)
    }
    let a = &b"ijklmn"[..];

    let res_a = err_test(a);
    assert_eq!(
        res_a,
        Err(ErrMode::Backtrack(error_position!(
            &&b"mn"[..],
            ErrorKind::Tag
        )))
    );
}

#[test]
fn separated_pair_test() {
    #[allow(clippy::type_complexity)]
    fn sep_pair_abc_def(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, (&[u8], &[u8])> {
        separated_pair("abc", ",", "def").parse_peek(i)
    }

    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"abc,defghijkl"[..])),
        Ok((Partial::new(&b"ghijkl"[..]), (&b"abc"[..], &b"def"[..])))
    );
    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"abc,d"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"xxx,def"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx,def"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        sep_pair_abc_def(Partial::new(&b"abc,xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
}

#[test]
fn preceded_test() {
    fn preceded_abcd_efgh(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        preceded("abcd", "efgh").parse_peek(i)
    }

    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"abcdefghijkl"[..])),
        Ok((Partial::new(&b"ijkl"[..]), &b"efgh"[..]))
    );
    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"abcde"[..])),
        Err(ErrMode::Incomplete(Needed::new(3)))
    );
    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"xxxxdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxxdef"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        preceded_abcd_efgh(Partial::new(&b"abcdxxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
}

#[test]
fn terminated_test() {
    fn terminated_abcd_efgh(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        terminated("abcd", "efgh").parse_peek(i)
    }

    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"abcdefghijkl"[..])),
        Ok((Partial::new(&b"ijkl"[..]), &b"abcd"[..]))
    );
    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"abcde"[..])),
        Err(ErrMode::Incomplete(Needed::new(3)))
    );
    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"xxxxdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxxdef"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        terminated_abcd_efgh(Partial::new(&b"abcdxxxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxx"[..]),
            ErrorKind::Tag
        )))
    );
}

#[test]
fn delimited_test() {
    fn delimited_abc_def_ghi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        delimited("abc", "def", "ghi").parse_peek(i)
    }

    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"abcdefghijkl"[..])),
        Ok((Partial::new(&b"jkl"[..]), &b"def"[..]))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"abcde"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"abcdefgh"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"xxxdefghi"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxdefghi"[..]),
            ErrorKind::Tag
        ),))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"abcxxxghi"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxghi"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        delimited_abc_def_ghi(Partial::new(&b"abcdefxxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
}

#[cfg(feature = "alloc")]
#[test]
fn alt_test() {
    #[cfg(feature = "alloc")]
    use crate::{
        error::ParserError,
        lib::std::{
            fmt::Debug,
            string::{String, ToString},
        },
    };

    #[cfg(feature = "alloc")]
    #[derive(Debug, Clone, Eq, PartialEq)]
    pub struct ErrorStr(String);

    #[cfg(feature = "alloc")]
    impl From<u32> for ErrorStr {
        fn from(i: u32) -> Self {
            ErrorStr(format!("custom error code: {}", i))
        }
    }

    #[cfg(feature = "alloc")]
    impl<'a> From<&'a str> for ErrorStr {
        fn from(i: &'a str) -> Self {
            ErrorStr(format!("custom error message: {}", i))
        }
    }

    #[cfg(feature = "alloc")]
    impl<I: Debug> ParserError<I> for ErrorStr {
        fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
            ErrorStr(format!("custom error message: ({:?}, {:?})", input, kind))
        }

        fn append(self, input: &I, kind: ErrorKind) -> Self {
            ErrorStr(format!(
                "custom error message: ({:?}, {:?}) - {:?}",
                input, kind, self
            ))
        }
    }

    fn work(input: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        Ok(input.peek_finish())
    }

    #[allow(unused_variables)]
    fn dont_work(input: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        Err(ErrMode::Backtrack(ErrorStr("abcd".to_string())))
    }

    fn work2(input: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        Ok((input, &b""[..]))
    }

    fn alt1(i: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        alt((unpeek(dont_work), unpeek(dont_work))).parse_peek(i)
    }
    fn alt2(i: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        alt((unpeek(dont_work), unpeek(work))).parse_peek(i)
    }
    fn alt3(i: &[u8]) -> IResult<&[u8], &[u8], ErrorStr> {
        alt((
            unpeek(dont_work),
            unpeek(dont_work),
            unpeek(work2),
            unpeek(dont_work),
        ))
        .parse_peek(i)
    }
    //named!(alt1, alt!(dont_work | dont_work));
    //named!(alt2, alt!(dont_work | work));
    //named!(alt3, alt!(dont_work | dont_work | work2 | dont_work));

    let a = &b"abcd"[..];
    assert_eq!(
        alt1(a),
        Err(ErrMode::Backtrack(error_node_position!(
            &a,
            ErrorKind::Alt,
            ErrorStr("abcd".to_string())
        )))
    );
    assert_eq!(alt2(a), Ok((&b""[..], a)));
    assert_eq!(alt3(a), Ok((a, &b""[..])));

    fn alt4(i: &[u8]) -> IResult<&[u8], &[u8]> {
        alt(("abcd", "efgh")).parse_peek(i)
    }
    let b = &b"efgh"[..];
    assert_eq!(alt4(a), Ok((&b""[..], a)));
    assert_eq!(alt4(b), Ok((&b""[..], b)));
}

#[test]
fn alt_incomplete() {
    fn alt1(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, &[u8]> {
        alt(("a", "bc", "def")).parse_peek(i)
    }

    let a = &b""[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    let a = &b"b"[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    let a = &b"bcd"[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Ok((Partial::new(&b"d"[..]), &b"bc"[..]))
    );
    let a = &b"cde"[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(a),
            ErrorKind::Tag
        )))
    );
    let a = &b"de"[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    let a = &b"defg"[..];
    assert_eq!(
        alt1(Partial::new(a)),
        Ok((Partial::new(&b"g"[..]), &b"def"[..]))
    );
}

#[test]
fn alt_array() {
    fn alt1<'i>(i: &mut &'i [u8]) -> PResult<&'i [u8]> {
        alt(["a", "bc", "def"]).parse_next(i)
    }

    let i = &b"a"[..];
    assert_eq!(alt1.parse_peek(i), Ok((&b""[..], (&b"a"[..]))));

    let i = &b"bc"[..];
    assert_eq!(alt1.parse_peek(i), Ok((&b""[..], (&b"bc"[..]))));

    let i = &b"defg"[..];
    assert_eq!(alt1.parse_peek(i), Ok((&b"g"[..], (&b"def"[..]))));

    let i = &b"z"[..];
    assert_eq!(
        alt1.parse_peek(i),
        Err(ErrMode::Backtrack(error_position!(&i, ErrorKind::Tag)))
    );
}

#[test]
fn permutation_test() {
    #[allow(clippy::type_complexity)]
    fn perm(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, (&[u8], &[u8], &[u8])> {
        permutation(("abcd", "efg", "hi")).parse_peek(i)
    }

    let expected = (&b"abcd"[..], &b"efg"[..], &b"hi"[..]);

    let a = &b"abcdefghijk"[..];
    assert_eq!(
        perm(Partial::new(a)),
        Ok((Partial::new(&b"jk"[..]), expected))
    );
    let b = &b"efgabcdhijk"[..];
    assert_eq!(
        perm(Partial::new(b)),
        Ok((Partial::new(&b"jk"[..]), expected))
    );
    let c = &b"hiefgabcdjk"[..];
    assert_eq!(
        perm(Partial::new(c)),
        Ok((Partial::new(&b"jk"[..]), expected))
    );

    let d = &b"efgxyzabcdefghi"[..];
    assert_eq!(
        perm(Partial::new(d)),
        Err(ErrMode::Backtrack(error_node_position!(
            &Partial::new(&b"efgxyzabcdefghi"[..]),
            ErrorKind::Alt,
            error_position!(&Partial::new(&b"xyzabcdefghi"[..]), ErrorKind::Tag)
        )))
    );

    let e = &b"efgabc"[..];
    assert_eq!(
        perm(Partial::new(e)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn separated0_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(0.., "abcd", ",").parse_peek(i)
    }
    fn multi_empty(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(0.., "", ",").parse_peek(i)
    }
    fn multi_longsep(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(0.., "abcd", "..").parse_peek(i)
    }

    let a = &b"abcdef"[..];
    let b = &b"abcd,abcdef"[..];
    let c = &b"azerty"[..];
    let d = &b",,abc"[..];
    let e = &b"abcd,abcd,ef"[..];
    let f = &b"abc"[..];
    let g = &b"abcd."[..];
    let h = &b"abcd,abc"[..];

    let res1 = vec![&b"abcd"[..]];
    assert_eq!(multi(Partial::new(a)), Ok((Partial::new(&b"ef"[..]), res1)));
    let res2 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(multi(Partial::new(b)), Ok((Partial::new(&b"ef"[..]), res2)));
    assert_eq!(
        multi(Partial::new(c)),
        Ok((Partial::new(&b"azerty"[..]), Vec::new()))
    );
    let res3 = vec![&b""[..], &b""[..], &b""[..]];
    assert_eq!(
        multi_empty(Partial::new(d)),
        Ok((Partial::new(&b"abc"[..]), res3))
    );
    let res4 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(e)),
        Ok((Partial::new(&b",ef"[..]), res4))
    );

    assert_eq!(
        multi(Partial::new(f)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        multi_longsep(Partial::new(g)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        multi(Partial::new(h)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
}

#[test]
#[cfg(feature = "alloc")]
#[cfg_attr(debug_assertions, should_panic)]
fn separated0_empty_sep_test() {
    fn empty_sep(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(0.., "abc", "").parse_peek(i)
    }

    let i = &b"abcabc"[..];

    let i_err_pos = &i[3..];
    assert_eq!(
        empty_sep(Partial::new(i)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(i_err_pos),
            ErrorKind::Assert
        )))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn separated1_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(1.., "abcd", ",").parse_peek(i)
    }
    fn multi_longsep(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(1.., "abcd", "..").parse_peek(i)
    }

    let a = &b"abcdef"[..];
    let b = &b"abcd,abcdef"[..];
    let c = &b"azerty"[..];
    let d = &b"abcd,abcd,ef"[..];

    let f = &b"abc"[..];
    let g = &b"abcd."[..];
    let h = &b"abcd,abc"[..];

    let res1 = vec![&b"abcd"[..]];
    assert_eq!(multi(Partial::new(a)), Ok((Partial::new(&b"ef"[..]), res1)));
    let res2 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(multi(Partial::new(b)), Ok((Partial::new(&b"ef"[..]), res2)));
    assert_eq!(
        multi(Partial::new(c)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(c),
            ErrorKind::Tag
        )))
    );
    let res3 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(d)),
        Ok((Partial::new(&b",ef"[..]), res3))
    );

    assert_eq!(
        multi(Partial::new(f)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        multi_longsep(Partial::new(g)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        multi(Partial::new(h)),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn separated_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        separated(2..=4, "abcd", ",").parse_peek(i)
    }

    let a = &b"abcd,ef"[..];
    let b = &b"abcd,abcd,efgh"[..];
    let c = &b"abcd,abcd,abcd,abcd,efgh"[..];
    let d = &b"abcd,abcd,abcd,abcd,abcd,efgh"[..];
    let e = &b"abcd,ab"[..];

    assert_eq!(
        multi(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"ef"[..]),
            ErrorKind::Tag
        )))
    );
    let res1 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(b)),
        Ok((Partial::new(&b",efgh"[..]), res1))
    );
    let res2 = vec![&b"abcd"[..], &b"abcd"[..], &b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(c)),
        Ok((Partial::new(&b",efgh"[..]), res2))
    );
    let res3 = vec![&b"abcd"[..], &b"abcd"[..], &b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(d)),
        Ok((Partial::new(&b",abcd,efgh"[..]), res3))
    );
    assert_eq!(
        multi(Partial::new(e)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn repeat0_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(0.., "abcd").parse_peek(i)
    }

    assert_eq!(
        multi(Partial::new(&b"abcdef"[..])),
        Ok((Partial::new(&b"ef"[..]), vec![&b"abcd"[..]]))
    );
    assert_eq!(
        multi(Partial::new(&b"abcdabcdefgh"[..])),
        Ok((Partial::new(&b"efgh"[..]), vec![&b"abcd"[..], &b"abcd"[..]]))
    );
    assert_eq!(
        multi(Partial::new(&b"azerty"[..])),
        Ok((Partial::new(&b"azerty"[..]), Vec::new()))
    );
    assert_eq!(
        multi(Partial::new(&b"abcdab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        multi(Partial::new(&b"abcd"[..])),
        Err(ErrMode::Incomplete(Needed::new(4)))
    );
    assert_eq!(
        multi(Partial::new(&b""[..])),
        Err(ErrMode::Incomplete(Needed::new(4)))
    );
}

#[test]
#[cfg(feature = "alloc")]
#[cfg_attr(debug_assertions, should_panic)]
fn repeat0_empty_test() {
    fn multi_empty(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(0.., "").parse_peek(i)
    }

    assert_eq!(
        multi_empty(Partial::new(&b"abcdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"abcdef"[..]),
            ErrorKind::Assert
        )))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn repeat1_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(1.., "abcd").parse_peek(i)
    }

    let a = &b"abcdef"[..];
    let b = &b"abcdabcdefgh"[..];
    let c = &b"azerty"[..];
    let d = &b"abcdab"[..];

    let res1 = vec![&b"abcd"[..]];
    assert_eq!(multi(Partial::new(a)), Ok((Partial::new(&b"ef"[..]), res1)));
    let res2 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(b)),
        Ok((Partial::new(&b"efgh"[..]), res2))
    );
    assert_eq!(
        multi(Partial::new(c)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(c),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        multi(Partial::new(d)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn repeat_till_test() {
    #[allow(clippy::type_complexity)]
    fn multi(i: &[u8]) -> IResult<&[u8], (Vec<&[u8]>, &[u8])> {
        repeat_till(0.., "abcd", "efgh").parse_peek(i)
    }

    let a = b"abcdabcdefghabcd";
    let b = b"efghabcd";
    let c = b"azerty";

    let res_a = (vec![&b"abcd"[..], &b"abcd"[..]], &b"efgh"[..]);
    let res_b: (Vec<&[u8]>, &[u8]) = (Vec::new(), &b"efgh"[..]);
    assert_eq!(multi(&a[..]), Ok((&b"abcd"[..], res_a)));
    assert_eq!(multi(&b[..]), Ok((&b"abcd"[..], res_b)));
    assert_eq!(
        multi(&c[..]),
        Err(ErrMode::Backtrack(error_node_position!(
            &&c[..],
            ErrorKind::Many,
            error_position!(&&c[..], ErrorKind::Tag)
        )))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn repeat_till_range_test() {
    #[allow(clippy::type_complexity)]
    fn multi(i: &str) -> IResult<&str, (Vec<&str>, &str)> {
        repeat_till(2..=4, "ab", "cd").parse_peek(i)
    }

    assert_eq!(
        multi("cd"),
        Err(ErrMode::Backtrack(error_node_position!(
            &"cd",
            ErrorKind::Many,
            error_position!(&"cd", ErrorKind::Tag)
        )))
    );
    assert_eq!(
        multi("abcd"),
        Err(ErrMode::Backtrack(error_node_position!(
            &"cd",
            ErrorKind::Many,
            error_position!(&"cd", ErrorKind::Tag)
        )))
    );
    assert_eq!(multi("ababcd"), Ok(("", (vec!["ab", "ab"], "cd"))));
    assert_eq!(multi("abababcd"), Ok(("", (vec!["ab", "ab", "ab"], "cd"))));
    assert_eq!(
        multi("ababababcd"),
        Ok(("", (vec!["ab", "ab", "ab", "ab"], "cd")))
    );
    assert_eq!(
        multi("abababababcd"),
        Err(ErrMode::Backtrack(error_node_position!(
            &"cd",
            ErrorKind::Many,
            error_position!(&"abcd", ErrorKind::Tag)
        )))
    );
}

#[test]
#[cfg(feature = "std")]
fn infinite_many() {
    fn tst(input: &[u8]) -> IResult<&[u8], &[u8]> {
        println!("input: {:?}", input);
        Err(ErrMode::Backtrack(error_position!(&input, ErrorKind::Tag)))
    }

    // should not go into an infinite loop
    fn multi0(i: &[u8]) -> IResult<&[u8], Vec<&[u8]>> {
        repeat(0.., unpeek(tst)).parse_peek(i)
    }
    let a = &b"abcdef"[..];
    assert_eq!(multi0(a), Ok((a, Vec::new())));

    fn multi1(i: &[u8]) -> IResult<&[u8], Vec<&[u8]>> {
        repeat(1.., unpeek(tst)).parse_peek(i)
    }
    let a = &b"abcdef"[..];
    assert_eq!(
        multi1(a),
        Err(ErrMode::Backtrack(error_position!(&a, ErrorKind::Tag)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn repeat_test() {
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(2..=4, "Abcd").parse_peek(i)
    }

    let a = &b"Abcdef"[..];
    let b = &b"AbcdAbcdefgh"[..];
    let c = &b"AbcdAbcdAbcdAbcdefgh"[..];
    let d = &b"AbcdAbcdAbcdAbcdAbcdefgh"[..];
    let e = &b"AbcdAb"[..];

    assert_eq!(
        multi(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"ef"[..]),
            ErrorKind::Tag
        )))
    );
    let res1 = vec![&b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(b)),
        Ok((Partial::new(&b"efgh"[..]), res1))
    );
    let res2 = vec![&b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(c)),
        Ok((Partial::new(&b"efgh"[..]), res2))
    );
    let res3 = vec![&b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(d)),
        Ok((Partial::new(&b"Abcdefgh"[..]), res3))
    );
    assert_eq!(
        multi(Partial::new(e)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn count_test() {
    const TIMES: usize = 2;
    fn cnt_2(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(TIMES, "abc").parse_peek(i)
    }

    assert_eq!(
        cnt_2(Partial::new(&b"abcabcabcdef"[..])),
        Ok((Partial::new(&b"abcdef"[..]), vec![&b"abc"[..], &b"abc"[..]]))
    );
    assert_eq!(
        cnt_2(Partial::new(&b"ab"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        cnt_2(Partial::new(&b"abcab"[..])),
        Err(ErrMode::Incomplete(Needed::new(1)))
    );
    assert_eq!(
        cnt_2(Partial::new(&b"xxx"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxx"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        cnt_2(Partial::new(&b"xxxabcabcdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxabcabcdef"[..]),
            ErrorKind::Tag
        )))
    );
    assert_eq!(
        cnt_2(Partial::new(&b"abcxxxabcdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"xxxabcdef"[..]),
            ErrorKind::Tag
        )))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn count_zero() {
    const TIMES: usize = 0;
    fn counter_2(i: &[u8]) -> IResult<&[u8], Vec<&[u8]>> {
        repeat(TIMES, "abc").parse_peek(i)
    }

    let done = &b"abcabcabcdef"[..];
    let parsed_done = Vec::new();
    let rest = done;
    let incomplete_1 = &b"ab"[..];
    let parsed_incompl_1 = Vec::new();
    let incomplete_2 = &b"abcab"[..];
    let parsed_incompl_2 = Vec::new();
    let error = &b"xxx"[..];
    let error_remain = &b"xxx"[..];
    let parsed_err = Vec::new();
    let error_1 = &b"xxxabcabcdef"[..];
    let parsed_err_1 = Vec::new();
    let error_1_remain = &b"xxxabcabcdef"[..];
    let error_2 = &b"abcxxxabcdef"[..];
    let parsed_err_2 = Vec::new();
    let error_2_remain = &b"abcxxxabcdef"[..];

    assert_eq!(counter_2(done), Ok((rest, parsed_done)));
    assert_eq!(
        counter_2(incomplete_1),
        Ok((incomplete_1, parsed_incompl_1))
    );
    assert_eq!(
        counter_2(incomplete_2),
        Ok((incomplete_2, parsed_incompl_2))
    );
    assert_eq!(counter_2(error), Ok((error_remain, parsed_err)));
    assert_eq!(counter_2(error_1), Ok((error_1_remain, parsed_err_1)));
    assert_eq!(counter_2(error_2), Ok((error_2_remain, parsed_err_2)));
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct NilError;

impl<I> From<(I, ErrorKind)> for NilError {
    fn from(_: (I, ErrorKind)) -> Self {
        NilError
    }
}

impl<I> ParserError<I> for NilError {
    fn from_error_kind(_: &I, _: ErrorKind) -> NilError {
        NilError
    }
    fn append(self, _: &I, _: ErrorKind) -> NilError {
        NilError
    }
}

#[test]
#[cfg(feature = "alloc")]
fn fold_repeat0_test() {
    fn fold_into_vec<T>(mut acc: Vec<T>, item: T) -> Vec<T> {
        acc.push(item);
        acc
    }
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(0.., "abcd")
            .fold(Vec::new, fold_into_vec)
            .parse_peek(i)
    }

    assert_eq!(
        multi(Partial::new(&b"abcdef"[..])),
        Ok((Partial::new(&b"ef"[..]), vec![&b"abcd"[..]]))
    );
    assert_eq!(
        multi(Partial::new(&b"abcdabcdefgh"[..])),
        Ok((Partial::new(&b"efgh"[..]), vec![&b"abcd"[..], &b"abcd"[..]]))
    );
    assert_eq!(
        multi(Partial::new(&b"azerty"[..])),
        Ok((Partial::new(&b"azerty"[..]), Vec::new()))
    );
    assert_eq!(
        multi(Partial::new(&b"abcdab"[..])),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
    assert_eq!(
        multi(Partial::new(&b"abcd"[..])),
        Err(ErrMode::Incomplete(Needed::new(4)))
    );
    assert_eq!(
        multi(Partial::new(&b""[..])),
        Err(ErrMode::Incomplete(Needed::new(4)))
    );
}

#[test]
#[cfg(feature = "alloc")]
#[cfg_attr(debug_assertions, should_panic)]
fn fold_repeat0_empty_test() {
    fn fold_into_vec<T>(mut acc: Vec<T>, item: T) -> Vec<T> {
        acc.push(item);
        acc
    }
    fn multi_empty(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(0.., "").fold(Vec::new, fold_into_vec).parse_peek(i)
    }

    assert_eq!(
        multi_empty(Partial::new(&b"abcdef"[..])),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"abcdef"[..]),
            ErrorKind::Assert
        )))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn fold_repeat1_test() {
    fn fold_into_vec<T>(mut acc: Vec<T>, item: T) -> Vec<T> {
        acc.push(item);
        acc
    }
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(1.., "abcd")
            .fold(Vec::new, fold_into_vec)
            .parse_peek(i)
    }

    let a = &b"abcdef"[..];
    let b = &b"abcdabcdefgh"[..];
    let c = &b"azerty"[..];
    let d = &b"abcdab"[..];

    let res1 = vec![&b"abcd"[..]];
    assert_eq!(multi(Partial::new(a)), Ok((Partial::new(&b"ef"[..]), res1)));
    let res2 = vec![&b"abcd"[..], &b"abcd"[..]];
    assert_eq!(
        multi(Partial::new(b)),
        Ok((Partial::new(&b"efgh"[..]), res2))
    );
    assert_eq!(
        multi(Partial::new(c)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(c),
            ErrorKind::Many
        )))
    );
    assert_eq!(
        multi(Partial::new(d)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
#[cfg(feature = "alloc")]
fn fold_repeat_test() {
    fn fold_into_vec<T>(mut acc: Vec<T>, item: T) -> Vec<T> {
        acc.push(item);
        acc
    }
    fn multi(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, Vec<&[u8]>> {
        repeat(2..=4, "Abcd")
            .fold(Vec::new, fold_into_vec)
            .parse_peek(i)
    }

    let a = &b"Abcdef"[..];
    let b = &b"AbcdAbcdefgh"[..];
    let c = &b"AbcdAbcdAbcdAbcdefgh"[..];
    let d = &b"AbcdAbcdAbcdAbcdAbcdefgh"[..];
    let e = &b"AbcdAb"[..];

    assert_eq!(
        multi(Partial::new(a)),
        Err(ErrMode::Backtrack(error_position!(
            &Partial::new(&b"ef"[..]),
            ErrorKind::Tag
        )))
    );
    let res1 = vec![&b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(b)),
        Ok((Partial::new(&b"efgh"[..]), res1))
    );
    let res2 = vec![&b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(c)),
        Ok((Partial::new(&b"efgh"[..]), res2))
    );
    let res3 = vec![&b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..], &b"Abcd"[..]];
    assert_eq!(
        multi(Partial::new(d)),
        Ok((Partial::new(&b"Abcdefgh"[..]), res3))
    );
    assert_eq!(
        multi(Partial::new(e)),
        Err(ErrMode::Incomplete(Needed::new(2)))
    );
}

#[test]
fn repeat0_count_test() {
    fn count0_nums(i: &[u8]) -> IResult<&[u8], usize> {
        repeat(0.., (digit, ",")).parse_peek(i)
    }

    assert_eq!(count0_nums(&b"123,junk"[..]), Ok((&b"junk"[..], 1)));

    assert_eq!(count0_nums(&b"123,45,junk"[..]), Ok((&b"junk"[..], 2)));

    assert_eq!(
        count0_nums(&b"1,2,3,4,5,6,7,8,9,0,junk"[..]),
        Ok((&b"junk"[..], 10))
    );

    assert_eq!(count0_nums(&b"hello"[..]), Ok((&b"hello"[..], 0)));
}

#[test]
fn repeat1_count_test() {
    fn count1_nums(i: &[u8]) -> IResult<&[u8], usize> {
        repeat(1.., (digit, ",")).parse_peek(i)
    }

    assert_eq!(count1_nums(&b"123,45,junk"[..]), Ok((&b"junk"[..], 2)));

    assert_eq!(
        count1_nums(&b"1,2,3,4,5,6,7,8,9,0,junk"[..]),
        Ok((&b"junk"[..], 10))
    );

    assert_eq!(
        count1_nums(&b"hello"[..]),
        Err(ErrMode::Backtrack(error_position!(
            &&b"hello"[..],
            ErrorKind::Slice
        )))
    );
}
