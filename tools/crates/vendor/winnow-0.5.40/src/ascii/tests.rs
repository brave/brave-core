use super::*;
use crate::prelude::*;

mod complete {
    use super::*;
    use crate::combinator::alt;
    use crate::combinator::opt;
    use crate::error::ErrMode;
    use crate::error::ErrorKind;
    use crate::error::InputError;
    use crate::stream::ParseSlice;
    use crate::token::none_of;
    use crate::token::one_of;
    #[cfg(feature = "alloc")]
    use crate::{lib::std::string::String, lib::std::vec::Vec};
    use proptest::prelude::*;

    macro_rules! assert_parse(
    ($left: expr, $right: expr) => {
      let res: $crate::IResult<_, _, InputError<_>> = $left;
      assert_eq!(res, $right);
    };
  );

    #[test]
    fn character() {
        let empty: &[u8] = b"";
        let a: &[u8] = b"abcd";
        let b: &[u8] = b"1234";
        let c: &[u8] = b"a123";
        let d: &[u8] = "azé12".as_bytes();
        let e: &[u8] = b" ";
        let f: &[u8] = b" ;";
        //assert_eq!(alpha1::<_, InputError>(a), Err(ErrMode::Incomplete(Needed::Size(1))));
        assert_parse!(alpha1.parse_peek(a), Ok((empty, a)));
        assert_eq!(
            alpha1.parse_peek(b),
            Err(ErrMode::Backtrack(InputError::new(b, ErrorKind::Slice)))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(c),
            Ok((&c[1..], &b"a"[..]))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(d),
            Ok(("é12".as_bytes(), &b"az"[..]))
        );
        assert_eq!(
            digit1.parse_peek(a),
            Err(ErrMode::Backtrack(InputError::new(a, ErrorKind::Slice)))
        );
        assert_eq!(digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(
            digit1.parse_peek(c),
            Err(ErrMode::Backtrack(InputError::new(c, ErrorKind::Slice)))
        );
        assert_eq!(
            digit1.parse_peek(d),
            Err(ErrMode::Backtrack(InputError::new(d, ErrorKind::Slice)))
        );
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(a), Ok((empty, a)));
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(c), Ok((empty, c)));
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(d),
            Ok(("zé12".as_bytes(), &b"a"[..]))
        );
        assert_eq!(
            hex_digit1.parse_peek(e),
            Err(ErrMode::Backtrack(InputError::new(e, ErrorKind::Slice)))
        );
        assert_eq!(
            oct_digit1.parse_peek(a),
            Err(ErrMode::Backtrack(InputError::new(a, ErrorKind::Slice)))
        );
        assert_eq!(oct_digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(
            oct_digit1.parse_peek(c),
            Err(ErrMode::Backtrack(InputError::new(c, ErrorKind::Slice)))
        );
        assert_eq!(
            oct_digit1.parse_peek(d),
            Err(ErrMode::Backtrack(InputError::new(d, ErrorKind::Slice)))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(a),
            Ok((empty, a))
        );
        //assert_eq!(fix_error!(b,(), alphanumeric), Ok((empty, b)));
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(c),
            Ok((empty, c))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(d),
            Ok(("é12".as_bytes(), &b"az"[..]))
        );
        assert_eq!(space1::<_, InputError<_>>.parse_peek(e), Ok((empty, e)));
        assert_eq!(
            space1::<_, InputError<_>>.parse_peek(f),
            Ok((&b";"[..], &b" "[..]))
        );
    }

    #[cfg(feature = "alloc")]
    #[test]
    fn character_s() {
        let empty = "";
        let a = "abcd";
        let b = "1234";
        let c = "a123";
        let d = "azé12";
        let e = " ";
        assert_eq!(alpha1::<_, InputError<_>>.parse_peek(a), Ok((empty, a)));
        assert_eq!(
            alpha1.parse_peek(b),
            Err(ErrMode::Backtrack(InputError::new(b, ErrorKind::Slice)))
        );
        assert_eq!(alpha1::<_, InputError<_>>.parse_peek(c), Ok((&c[1..], "a")));
        assert_eq!(alpha1::<_, InputError<_>>.parse_peek(d), Ok(("é12", "az")));
        assert_eq!(
            digit1.parse_peek(a),
            Err(ErrMode::Backtrack(InputError::new(a, ErrorKind::Slice)))
        );
        assert_eq!(digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(
            digit1.parse_peek(c),
            Err(ErrMode::Backtrack(InputError::new(c, ErrorKind::Slice)))
        );
        assert_eq!(
            digit1.parse_peek(d),
            Err(ErrMode::Backtrack(InputError::new(d, ErrorKind::Slice)))
        );
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(a), Ok((empty, a)));
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(hex_digit1::<_, InputError<_>>.parse_peek(c), Ok((empty, c)));
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(d),
            Ok(("zé12", "a"))
        );
        assert_eq!(
            hex_digit1.parse_peek(e),
            Err(ErrMode::Backtrack(InputError::new(e, ErrorKind::Slice)))
        );
        assert_eq!(
            oct_digit1.parse_peek(a),
            Err(ErrMode::Backtrack(InputError::new(a, ErrorKind::Slice)))
        );
        assert_eq!(oct_digit1::<_, InputError<_>>.parse_peek(b), Ok((empty, b)));
        assert_eq!(
            oct_digit1.parse_peek(c),
            Err(ErrMode::Backtrack(InputError::new(c, ErrorKind::Slice)))
        );
        assert_eq!(
            oct_digit1.parse_peek(d),
            Err(ErrMode::Backtrack(InputError::new(d, ErrorKind::Slice)))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(a),
            Ok((empty, a))
        );
        //assert_eq!(fix_error!(b,(), alphanumeric), Ok((empty, b)));
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(c),
            Ok((empty, c))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(d),
            Ok(("é12", "az"))
        );
        assert_eq!(space1::<_, InputError<_>>.parse_peek(e), Ok((empty, e)));
    }

    use crate::stream::Offset;
    #[test]
    fn offset() {
        let a = &b"abcd;"[..];
        let b = &b"1234;"[..];
        let c = &b"a123;"[..];
        let d = &b" \t;"[..];
        let e = &b" \t\r\n;"[..];
        let f = &b"123abcDEF;"[..];

        match alpha1::<_, InputError<_>>.parse_peek(a) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&a) + i.len(), a.len());
            }
            _ => panic!("wrong return type in offset test for alpha"),
        }
        match digit1::<_, InputError<_>>.parse_peek(b) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&b) + i.len(), b.len());
            }
            _ => panic!("wrong return type in offset test for digit"),
        }
        match alphanumeric1::<_, InputError<_>>.parse_peek(c) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&c) + i.len(), c.len());
            }
            _ => panic!("wrong return type in offset test for alphanumeric"),
        }
        match space1::<_, InputError<_>>.parse_peek(d) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&d) + i.len(), d.len());
            }
            _ => panic!("wrong return type in offset test for space"),
        }
        match multispace1::<_, InputError<_>>.parse_peek(e) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&e) + i.len(), e.len());
            }
            _ => panic!("wrong return type in offset test for multispace"),
        }
        match hex_digit1::<_, InputError<_>>.parse_peek(f) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&f) + i.len(), f.len());
            }
            _ => panic!("wrong return type in offset test for hex_digit"),
        }
        match oct_digit1::<_, InputError<_>>.parse_peek(f) {
            Ok((i, _)) => {
                assert_eq!(i.offset_from(&f) + i.len(), f.len());
            }
            _ => panic!("wrong return type in offset test for oct_digit"),
        }
    }

    #[test]
    fn is_till_line_ending_bytes() {
        let a: &[u8] = b"ab12cd\nefgh";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(a),
            Ok((&b"\nefgh"[..], &b"ab12cd"[..]))
        );

        let b: &[u8] = b"ab12cd\nefgh\nijkl";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(b),
            Ok((&b"\nefgh\nijkl"[..], &b"ab12cd"[..]))
        );

        let c: &[u8] = b"ab12cd\r\nefgh\nijkl";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(c),
            Ok((&b"\r\nefgh\nijkl"[..], &b"ab12cd"[..]))
        );

        let d: &[u8] = b"ab12cd";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(d),
            Ok((&[][..], d))
        );
    }

    #[test]
    fn is_till_line_ending_str() {
        let f = "βèƒôřè\rÂßÇáƒƭèř";
        assert_eq!(
            till_line_ending.parse_peek(f),
            Err(ErrMode::Backtrack(InputError::new(
                &f[12..],
                ErrorKind::Tag
            )))
        );

        let g2: &str = "ab12cd";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(g2),
            Ok(("", g2))
        );
    }

    #[test]
    fn hex_digit_test() {
        let i = &b"0123456789abcdefABCDEF;"[..];
        assert_parse!(hex_digit1.parse_peek(i), Ok((&b";"[..], &i[..i.len() - 1])));

        let i = &b"g"[..];
        assert_parse!(
            hex_digit1.parse_peek(i),
            Err(ErrMode::Backtrack(error_position!(&i, ErrorKind::Slice)))
        );

        let i = &b"G"[..];
        assert_parse!(
            hex_digit1.parse_peek(i),
            Err(ErrMode::Backtrack(error_position!(&i, ErrorKind::Slice)))
        );

        assert!(AsChar::is_hex_digit(b'0'));
        assert!(AsChar::is_hex_digit(b'9'));
        assert!(AsChar::is_hex_digit(b'a'));
        assert!(AsChar::is_hex_digit(b'f'));
        assert!(AsChar::is_hex_digit(b'A'));
        assert!(AsChar::is_hex_digit(b'F'));
        assert!(!AsChar::is_hex_digit(b'g'));
        assert!(!AsChar::is_hex_digit(b'G'));
        assert!(!AsChar::is_hex_digit(b'/'));
        assert!(!AsChar::is_hex_digit(b':'));
        assert!(!AsChar::is_hex_digit(b'@'));
        assert!(!AsChar::is_hex_digit(b'\x60'));
    }

    #[test]
    fn oct_digit_test() {
        let i = &b"01234567;"[..];
        assert_parse!(oct_digit1.parse_peek(i), Ok((&b";"[..], &i[..i.len() - 1])));

        let i = &b"8"[..];
        assert_parse!(
            oct_digit1.parse_peek(i),
            Err(ErrMode::Backtrack(error_position!(&i, ErrorKind::Slice)))
        );

        assert!(AsChar::is_oct_digit(b'0'));
        assert!(AsChar::is_oct_digit(b'7'));
        assert!(!AsChar::is_oct_digit(b'8'));
        assert!(!AsChar::is_oct_digit(b'9'));
        assert!(!AsChar::is_oct_digit(b'a'));
        assert!(!AsChar::is_oct_digit(b'A'));
        assert!(!AsChar::is_oct_digit(b'/'));
        assert!(!AsChar::is_oct_digit(b':'));
        assert!(!AsChar::is_oct_digit(b'@'));
        assert!(!AsChar::is_oct_digit(b'\x60'));
    }

    #[test]
    fn full_line_windows() {
        fn take_full_line(i: &[u8]) -> IResult<&[u8], (&[u8], &[u8])> {
            (till_line_ending, line_ending).parse_peek(i)
        }
        let input = b"abc\r\n";
        let output = take_full_line(input);
        assert_eq!(output, Ok((&b""[..], (&b"abc"[..], &b"\r\n"[..]))));
    }

    #[test]
    fn full_line_unix() {
        fn take_full_line(i: &[u8]) -> IResult<&[u8], (&[u8], &[u8])> {
            (till_line_ending, line_ending).parse_peek(i)
        }
        let input = b"abc\n";
        let output = take_full_line(input);
        assert_eq!(output, Ok((&b""[..], (&b"abc"[..], &b"\n"[..]))));
    }

    #[test]
    fn check_windows_lineending() {
        let input = b"\r\n";
        let output = line_ending.parse_peek(&input[..]);
        assert_parse!(output, Ok((&b""[..], &b"\r\n"[..])));
    }

    #[test]
    fn check_unix_lineending() {
        let input = b"\n";
        let output = line_ending.parse_peek(&input[..]);
        assert_parse!(output, Ok((&b""[..], &b"\n"[..])));
    }

    #[test]
    fn cr_lf() {
        assert_parse!(
            crlf.parse_peek(&b"\r\na"[..]),
            Ok((&b"a"[..], &b"\r\n"[..]))
        );
        assert_parse!(
            crlf.parse_peek(&b"\r"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b"\r"[..],
                ErrorKind::Tag
            )))
        );
        assert_parse!(
            crlf.parse_peek(&b"\ra"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b"\ra"[..],
                ErrorKind::Tag
            )))
        );

        assert_parse!(crlf.parse_peek("\r\na"), Ok(("a", "\r\n")));
        assert_parse!(
            crlf.parse_peek("\r"),
            Err(ErrMode::Backtrack(error_position!(&"\r", ErrorKind::Tag)))
        );
        assert_parse!(
            crlf.parse_peek("\ra"),
            Err(ErrMode::Backtrack(error_position!(&"\ra", ErrorKind::Tag)))
        );
    }

    #[test]
    fn end_of_line() {
        assert_parse!(
            line_ending.parse_peek(&b"\na"[..]),
            Ok((&b"a"[..], &b"\n"[..]))
        );
        assert_parse!(
            line_ending.parse_peek(&b"\r\na"[..]),
            Ok((&b"a"[..], &b"\r\n"[..]))
        );
        assert_parse!(
            line_ending.parse_peek(&b"\r"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b"\r"[..],
                ErrorKind::Tag
            )))
        );
        assert_parse!(
            line_ending.parse_peek(&b"\ra"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b"\ra"[..],
                ErrorKind::Tag
            )))
        );

        assert_parse!(line_ending.parse_peek("\na"), Ok(("a", "\n")));
        assert_parse!(line_ending.parse_peek("\r\na"), Ok(("a", "\r\n")));
        assert_parse!(
            line_ending.parse_peek("\r"),
            Err(ErrMode::Backtrack(error_position!(&"\r", ErrorKind::Tag)))
        );
        assert_parse!(
            line_ending.parse_peek("\ra"),
            Err(ErrMode::Backtrack(error_position!(&"\ra", ErrorKind::Tag)))
        );
    }

    fn digit_to_i16(input: &str) -> IResult<&str, i16> {
        let i = input;
        let (i, opt_sign) = opt(alt(('+', '-'))).parse_peek(i)?;
        let sign = match opt_sign {
            Some('+') | None => true,
            Some('-') => false,
            _ => unreachable!(),
        };

        let (i, s) = digit1::<_, InputError<_>>.parse_peek(i)?;
        match s.parse_slice() {
            Some(n) => {
                if sign {
                    Ok((i, n))
                } else {
                    Ok((i, -n))
                }
            }
            None => Err(ErrMode::from_error_kind(&i, ErrorKind::Verify)),
        }
    }

    fn digit_to_u32(i: &str) -> IResult<&str, u32> {
        let (i, s) = digit1.parse_peek(i)?;
        match s.parse_slice() {
            Some(n) => Ok((i, n)),
            None => Err(ErrMode::from_error_kind(&i, ErrorKind::Verify)),
        }
    }

    proptest! {
      #[test]
      #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
      fn ints(s in "\\PC*") {
          let res1 = digit_to_i16(&s);
          let res2 = dec_int.parse_peek(s.as_str());
          assert_eq!(res1, res2);
      }

      #[test]
      #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
      fn uints(s in "\\PC*") {
          let res1 = digit_to_u32(&s);
          let res2 = dec_uint.parse_peek(s.as_str());
          assert_eq!(res1, res2);
      }
    }

    #[test]
    fn hex_uint_tests() {
        fn hex_u32(input: &[u8]) -> IResult<&[u8], u32> {
            hex_uint.parse_peek(input)
        }

        assert_parse!(
            hex_u32(&b";"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b";"[..],
                ErrorKind::Slice
            )))
        );
        assert_parse!(hex_u32(&b"ff;"[..]), Ok((&b";"[..], 255)));
        assert_parse!(hex_u32(&b"1be2;"[..]), Ok((&b";"[..], 7138)));
        assert_parse!(hex_u32(&b"c5a31be2;"[..]), Ok((&b";"[..], 3_315_801_058)));
        assert_parse!(hex_u32(&b"C5A31be2;"[..]), Ok((&b";"[..], 3_315_801_058)));
        assert_parse!(
            hex_u32(&b"00c5a31be2;"[..]), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &&b"00c5a31be2;"[..],
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(&b"c5a31be201;"[..]), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &&b"c5a31be201;"[..],
                ErrorKind::Verify
            )))
        );
        assert_parse!(hex_u32(&b"ffffffff;"[..]), Ok((&b";"[..], 4_294_967_295)));
        assert_parse!(
            hex_u32(&b"ffffffffffffffff;"[..]), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &&b"ffffffffffffffff;"[..],
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(&b"ffffffffffffffff"[..]), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &&b"ffffffffffffffff"[..],
                ErrorKind::Verify
            )))
        );
        assert_parse!(hex_u32(&b"0x1be2;"[..]), Ok((&b"x1be2;"[..], 0)));
        assert_parse!(hex_u32(&b"12af"[..]), Ok((&b""[..], 0x12af)));
    }

    #[test]
    #[cfg(feature = "std")]
    fn float_test() {
        let test_cases = [
            "+3.14",
            "3.14",
            "-3.14",
            "0",
            "0.0",
            "1.",
            ".789",
            "-.5",
            "1e7",
            "-1E-7",
            ".3e-2",
            "1.e4",
            "1.2e4",
            "12.34",
            "-1.234E-12",
            "-1.234e-12",
            "0.00000000000000000087",
            "inf",
            "Inf",
            "infinity",
            "Infinity",
            "-inf",
            "-Inf",
            "-infinity",
            "-Infinity",
            "+inf",
            "+Inf",
            "+infinity",
            "+Infinity",
        ];

        for test in test_cases {
            let expected32 = str::parse::<f32>(test).unwrap();
            let expected64 = str::parse::<f64>(test).unwrap();

            println!("now parsing: {} -> {}", test, expected32);

            assert_parse!(
                float.parse_peek(test.as_bytes()),
                Ok((&b""[..], expected32))
            );
            assert_parse!(float.parse_peek(test), Ok(("", expected32)));

            assert_parse!(
                float.parse_peek(test.as_bytes()),
                Ok((&b""[..], expected64))
            );
            assert_parse!(float.parse_peek(test), Ok(("", expected64)));
        }

        let remaining_exponent = "-1.234E-";
        assert_parse!(
            float::<_, f64, _>.parse_peek(remaining_exponent),
            Err(ErrMode::Cut(InputError::new("", ErrorKind::Slice)))
        );

        let nan_test_cases = ["nan", "NaN", "NAN"];

        for test in nan_test_cases {
            println!("now parsing: {}", test);

            let (remaining, parsed) = float::<_, f32, ()>.parse_peek(test.as_bytes()).unwrap();
            assert!(parsed.is_nan());
            assert!(remaining.is_empty());

            let (remaining, parsed) = float::<_, f32, ()>.parse_peek(test).unwrap();
            assert!(parsed.is_nan());
            assert!(remaining.is_empty());

            let (remaining, parsed) = float::<_, f64, ()>.parse_peek(test.as_bytes()).unwrap();
            assert!(parsed.is_nan());
            assert!(remaining.is_empty());

            let (remaining, parsed) = float::<_, f64, ()>.parse_peek(test).unwrap();
            assert!(parsed.is_nan());
            assert!(remaining.is_empty());
        }
    }

    #[cfg(feature = "std")]
    fn parse_f64(i: &str) -> IResult<&str, f64, ()> {
        match super::recognize_float_or_exceptions.parse_peek(i) {
            Err(e) => Err(e),
            Ok((i, s)) => {
                if s.is_empty() {
                    return Err(ErrMode::Backtrack(()));
                }
                match s.parse_slice() {
                    Some(n) => Ok((i, n)),
                    None => Err(ErrMode::Backtrack(())),
                }
            }
        }
    }

    proptest! {
      #[test]
      #[cfg(feature = "std")]
      #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
      fn floats(s in "\\PC*") {
          println!("testing {}", s);
          let res1 = parse_f64(&s);
          let res2 = float::<_, f64, ()>.parse_peek(s.as_str());
          assert_eq!(res1, res2);
      }
    }

    // issue #1336 "escaped hangs if normal parser accepts empty"
    #[test]
    fn complete_escaped_hang() {
        // issue #1336 "escaped hangs if normal parser accepts empty"
        fn escaped_string(input: &str) -> IResult<&str, &str> {
            use crate::ascii::alpha0;
            use crate::token::one_of;
            escaped(alpha0, '\\', one_of(['n'])).parse_peek(input)
        }

        escaped_string("7").unwrap();
        escaped_string("a7").unwrap();
    }

    #[test]
    fn complete_escaped_hang_1118() {
        // issue ##1118 escaped does not work with empty string
        fn unquote(input: &str) -> IResult<&str, &str> {
            use crate::combinator::delimited;
            use crate::combinator::opt;
            use crate::token::one_of;

            delimited(
                '"',
                escaped(
                    opt(none_of(['\\', '"'])),
                    '\\',
                    one_of(['\\', '"', 'r', 'n', 't']),
                ),
                '"',
            )
            .parse_peek(input)
        }

        assert_eq!(unquote(r#""""#), Ok(("", "")));
    }

    #[cfg(feature = "alloc")]
    #[allow(unused_variables)]
    #[test]
    fn complete_escaping() {
        use crate::ascii::{alpha1 as alpha, digit1 as digit};
        use crate::token::one_of;

        fn esc(i: &[u8]) -> IResult<&[u8], &[u8]> {
            escaped(alpha, '\\', one_of(['\"', 'n', '\\'])).parse_peek(i)
        }
        assert_eq!(esc(&b"abcd;"[..]), Ok((&b";"[..], &b"abcd"[..])));
        assert_eq!(esc(&b"ab\\\"cd;"[..]), Ok((&b";"[..], &b"ab\\\"cd"[..])));
        assert_eq!(esc(&b"\\\"abcd;"[..]), Ok((&b";"[..], &b"\\\"abcd"[..])));
        assert_eq!(esc(&b"\\n;"[..]), Ok((&b";"[..], &b"\\n"[..])));
        assert_eq!(esc(&b"ab\\\"12"[..]), Ok((&b"12"[..], &b"ab\\\""[..])));
        assert_eq!(
            esc(&b"AB\\"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b""[..],
                ErrorKind::Token
            )))
        );
        assert_eq!(
            esc(&b"AB\\A"[..]),
            Err(ErrMode::Backtrack(error_node_position!(
                &&b"AB\\A"[..],
                ErrorKind::Token,
                error_position!(&&b"A"[..], ErrorKind::Verify)
            )))
        );

        fn esc2(i: &[u8]) -> IResult<&[u8], &[u8]> {
            escaped(digit, '\\', one_of(['\"', 'n', '\\'])).parse_peek(i)
        }
        assert_eq!(esc2(&b"12\\nnn34"[..]), Ok((&b"nn34"[..], &b"12\\n"[..])));
    }

    #[cfg(feature = "alloc")]
    #[test]
    fn complete_escaping_str() {
        use crate::ascii::{alpha1 as alpha, digit1 as digit};
        use crate::token::one_of;

        fn esc(i: &str) -> IResult<&str, &str> {
            escaped(alpha, '\\', one_of(['\"', 'n', '\\'])).parse_peek(i)
        }
        assert_eq!(esc("abcd;"), Ok((";", "abcd")));
        assert_eq!(esc("ab\\\"cd;"), Ok((";", "ab\\\"cd")));
        assert_eq!(esc("\\\"abcd;"), Ok((";", "\\\"abcd")));
        assert_eq!(esc("\\n;"), Ok((";", "\\n")));
        assert_eq!(esc("ab\\\"12"), Ok(("12", "ab\\\"")));
        assert_eq!(
            esc("AB\\"),
            Err(ErrMode::Backtrack(error_position!(&"", ErrorKind::Token)))
        );
        assert_eq!(
            esc("AB\\A"),
            Err(ErrMode::Backtrack(error_node_position!(
                &"AB\\A",
                ErrorKind::Token,
                error_position!(&"A", ErrorKind::Verify)
            )))
        );

        fn esc2(i: &str) -> IResult<&str, &str> {
            escaped(digit, '\\', one_of(['\"', 'n', '\\'])).parse_peek(i)
        }
        assert_eq!(esc2("12\\nnn34"), Ok(("nn34", "12\\n")));

        fn esc3(i: &str) -> IResult<&str, &str> {
            escaped(alpha, '\u{241b}', one_of(['\"', 'n'])).parse_peek(i)
        }
        assert_eq!(esc3("ab␛ncd;"), Ok((";", "ab␛ncd")));
    }

    #[test]
    fn test_escaped_error() {
        fn esc(s: &str) -> IResult<&str, &str> {
            use crate::ascii::digit1;
            escaped(digit1, '\\', one_of(['\"', 'n', '\\'])).parse_peek(s)
        }

        assert_eq!(esc("abcd"), Ok(("abcd", "")));
    }

    #[cfg(feature = "alloc")]
    #[test]
    fn complete_escape_transform() {
        use crate::ascii::alpha1 as alpha;

        #[cfg(feature = "alloc")]
        fn to_s(i: Vec<u8>) -> String {
            String::from_utf8_lossy(&i).into_owned()
        }

        fn esc(i: &[u8]) -> IResult<&[u8], String> {
            escaped_transform(
                alpha,
                '\\',
                alt((
                    "\\".value(&b"\\"[..]),
                    "\"".value(&b"\""[..]),
                    "n".value(&b"\n"[..]),
                )),
            )
            .map(to_s)
            .parse_peek(i)
        }

        assert_eq!(esc(&b"abcd;"[..]), Ok((&b";"[..], String::from("abcd"))));
        assert_eq!(
            esc(&b"ab\\\"cd;"[..]),
            Ok((&b";"[..], String::from("ab\"cd")))
        );
        assert_eq!(
            esc(&b"\\\"abcd;"[..]),
            Ok((&b";"[..], String::from("\"abcd")))
        );
        assert_eq!(esc(&b"\\n;"[..]), Ok((&b";"[..], String::from("\n"))));
        assert_eq!(
            esc(&b"ab\\\"12"[..]),
            Ok((&b"12"[..], String::from("ab\"")))
        );
        assert_eq!(
            esc(&b"AB\\"[..]),
            Err(ErrMode::Backtrack(error_position!(
                &&b""[..],
                ErrorKind::Tag
            )))
        );
        assert_eq!(
            esc(&b"AB\\A"[..]),
            Err(ErrMode::Backtrack(error_node_position!(
                &&b"AB\\A"[..],
                ErrorKind::Eof,
                error_position!(&&b"A"[..], ErrorKind::Tag)
            )))
        );

        fn esc2(i: &[u8]) -> IResult<&[u8], String> {
            escaped_transform(
                alpha,
                '&',
                alt((
                    "egrave;".value("è".as_bytes()),
                    "agrave;".value("à".as_bytes()),
                )),
            )
            .map(to_s)
            .parse_peek(i)
        }
        assert_eq!(
            esc2(&b"ab&egrave;DEF;"[..]),
            Ok((&b";"[..], String::from("abèDEF")))
        );
        assert_eq!(
            esc2(&b"ab&egrave;D&agrave;EF;"[..]),
            Ok((&b";"[..], String::from("abèDàEF")))
        );
    }

    #[cfg(feature = "std")]
    #[test]
    fn complete_escape_transform_str() {
        use crate::ascii::alpha1 as alpha;

        fn esc(i: &str) -> IResult<&str, String> {
            escaped_transform(
                alpha,
                '\\',
                alt(("\\".value("\\"), "\"".value("\""), "n".value("\n"))),
            )
            .parse_peek(i)
        }

        assert_eq!(esc("abcd;"), Ok((";", String::from("abcd"))));
        assert_eq!(esc("ab\\\"cd;"), Ok((";", String::from("ab\"cd"))));
        assert_eq!(esc("\\\"abcd;"), Ok((";", String::from("\"abcd"))));
        assert_eq!(esc("\\n;"), Ok((";", String::from("\n"))));
        assert_eq!(esc("ab\\\"12"), Ok(("12", String::from("ab\""))));
        assert_eq!(
            esc("AB\\"),
            Err(ErrMode::Backtrack(error_position!(&"", ErrorKind::Tag)))
        );
        assert_eq!(
            esc("AB\\A"),
            Err(ErrMode::Backtrack(error_node_position!(
                &"AB\\A",
                ErrorKind::Eof,
                error_position!(&"A", ErrorKind::Tag)
            )))
        );

        fn esc2(i: &str) -> IResult<&str, String> {
            escaped_transform(
                alpha,
                '&',
                alt(("egrave;".value("è"), "agrave;".value("à"))),
            )
            .parse_peek(i)
        }
        assert_eq!(esc2("ab&egrave;DEF;"), Ok((";", String::from("abèDEF"))));
        assert_eq!(
            esc2("ab&egrave;D&agrave;EF;"),
            Ok((";", String::from("abèDàEF")))
        );

        fn esc3(i: &str) -> IResult<&str, String> {
            escaped_transform(alpha, '␛', alt(("0".value("\0"), "n".value("\n")))).parse_peek(i)
        }
        assert_eq!(esc3("a␛0bc␛n"), Ok(("", String::from("a\0bc\n"))));
    }

    #[test]
    #[cfg(feature = "alloc")]
    fn test_escaped_transform_error() {
        fn esc_trans(s: &str) -> IResult<&str, String> {
            use crate::ascii::digit1;
            escaped_transform(digit1, '\\', "n").parse_peek(s)
        }

        assert_eq!(esc_trans("abcd"), Ok(("abcd", String::new())));
    }
}

mod partial {
    use super::*;
    use crate::combinator::opt;
    use crate::error::ErrorKind;
    use crate::error::InputError;
    use crate::error::{ErrMode, Needed};
    use crate::stream::ParseSlice;
    use crate::IResult;
    use crate::Partial;
    use proptest::prelude::*;

    macro_rules! assert_parse(
    ($left: expr, $right: expr) => {
      let res: $crate::IResult<_, _, InputError<_>> = $left;
      assert_eq!(res, $right);
    };
  );

    #[test]
    fn character() {
        let a: &[u8] = b"abcd";
        let b: &[u8] = b"1234";
        let c: &[u8] = b"a123";
        let d: &[u8] = "azé12".as_bytes();
        let e: &[u8] = b" ";
        let f: &[u8] = b" ;";
        //assert_eq!(alpha1::<_, Error<_>>(a), Err(ErrMode::Incomplete(Needed::new(1))));
        assert_parse!(
            alpha1.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            alpha1.parse_peek(Partial::new(b)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(b),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Ok((Partial::new(&c[1..]), &b"a"[..]))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("é12".as_bytes()), &b"az"[..]))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(a)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(a),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(c)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(c),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(d)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(d),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("zé12".as_bytes()), &b"a"[..]))
        );
        assert_eq!(
            hex_digit1.parse_peek(Partial::new(e)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(e),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(a)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(a),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(c)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(c),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(d)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(d),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        //assert_eq!(fix_error!(b,(), alphanumeric1), Ok((empty, b)));
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("é12".as_bytes()), &b"az"[..]))
        );
        assert_eq!(
            space1::<_, InputError<_>>.parse_peek(Partial::new(e)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            space1::<_, InputError<_>>.parse_peek(Partial::new(f)),
            Ok((Partial::new(&b";"[..]), &b" "[..]))
        );
    }

    #[cfg(feature = "alloc")]
    #[test]
    fn character_s() {
        let a = "abcd";
        let b = "1234";
        let c = "a123";
        let d = "azé12";
        let e = " ";
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            alpha1.parse_peek(Partial::new(b)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(b),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Ok((Partial::new(&c[1..]), "a"))
        );
        assert_eq!(
            alpha1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("é12"), "az"))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(a)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(a),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(c)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(c),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            digit1.parse_peek(Partial::new(d)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(d),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("zé12"), "a"))
        );
        assert_eq!(
            hex_digit1.parse_peek(Partial::new(e)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(e),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(a)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(a),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(c)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(c),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            oct_digit1.parse_peek(Partial::new(d)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(d),
                ErrorKind::Slice
            )))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        //assert_eq!(fix_error!(b,(), alphanumeric1), Ok((empty, b)));
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_eq!(
            alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Ok((Partial::new("é12"), "az"))
        );
        assert_eq!(
            space1::<_, InputError<_>>.parse_peek(Partial::new(e)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
    }

    use crate::stream::Offset;
    #[test]
    fn offset() {
        let a = &b"abcd;"[..];
        let b = &b"1234;"[..];
        let c = &b"a123;"[..];
        let d = &b" \t;"[..];
        let e = &b" \t\r\n;"[..];
        let f = &b"123abcDEF;"[..];

        match alpha1::<_, InputError<_>>.parse_peek(Partial::new(a)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&a) + i.len(), a.len());
            }
            _ => panic!("wrong return type in offset test for alpha"),
        }
        match digit1::<_, InputError<_>>.parse_peek(Partial::new(b)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&b) + i.len(), b.len());
            }
            _ => panic!("wrong return type in offset test for digit"),
        }
        match alphanumeric1::<_, InputError<_>>.parse_peek(Partial::new(c)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&c) + i.len(), c.len());
            }
            _ => panic!("wrong return type in offset test for alphanumeric"),
        }
        match space1::<_, InputError<_>>.parse_peek(Partial::new(d)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&d) + i.len(), d.len());
            }
            _ => panic!("wrong return type in offset test for space"),
        }
        match multispace1::<_, InputError<_>>.parse_peek(Partial::new(e)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&e) + i.len(), e.len());
            }
            _ => panic!("wrong return type in offset test for multispace"),
        }
        match hex_digit1::<_, InputError<_>>.parse_peek(Partial::new(f)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&f) + i.len(), f.len());
            }
            _ => panic!("wrong return type in offset test for hex_digit"),
        }
        match oct_digit1::<_, InputError<_>>.parse_peek(Partial::new(f)) {
            Ok((i, _)) => {
                let i = i.into_inner();
                assert_eq!(i.offset_from(&f) + i.len(), f.len());
            }
            _ => panic!("wrong return type in offset test for oct_digit"),
        }
    }

    #[test]
    fn is_till_line_ending_bytes() {
        let a: &[u8] = b"ab12cd\nefgh";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(Partial::new(a)),
            Ok((Partial::new(&b"\nefgh"[..]), &b"ab12cd"[..]))
        );

        let b: &[u8] = b"ab12cd\nefgh\nijkl";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(Partial::new(b)),
            Ok((Partial::new(&b"\nefgh\nijkl"[..]), &b"ab12cd"[..]))
        );

        let c: &[u8] = b"ab12cd\r\nefgh\nijkl";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(Partial::new(c)),
            Ok((Partial::new(&b"\r\nefgh\nijkl"[..]), &b"ab12cd"[..]))
        );

        let d: &[u8] = b"ab12cd";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(Partial::new(d)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
    }

    #[test]
    fn is_till_line_ending_str() {
        let f = "βèƒôřè\rÂßÇáƒƭèř";
        assert_eq!(
            till_line_ending.parse_peek(Partial::new(f)),
            Err(ErrMode::Backtrack(InputError::new(
                Partial::new(&f[12..]),
                ErrorKind::Tag
            )))
        );

        let g2: &str = "ab12cd";
        assert_eq!(
            till_line_ending::<_, InputError<_>>.parse_peek(Partial::new(g2)),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
    }

    #[test]
    fn hex_digit_test() {
        let i = &b"0123456789abcdefABCDEF;"[..];
        assert_parse!(
            hex_digit1.parse_peek(Partial::new(i)),
            Ok((Partial::new(&b";"[..]), &i[..i.len() - 1]))
        );

        let i = &b"g"[..];
        assert_parse!(
            hex_digit1.parse_peek(Partial::new(i)),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(i),
                ErrorKind::Slice
            )))
        );

        let i = &b"G"[..];
        assert_parse!(
            hex_digit1.parse_peek(Partial::new(i)),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(i),
                ErrorKind::Slice
            )))
        );

        assert!(AsChar::is_hex_digit(b'0'));
        assert!(AsChar::is_hex_digit(b'9'));
        assert!(AsChar::is_hex_digit(b'a'));
        assert!(AsChar::is_hex_digit(b'f'));
        assert!(AsChar::is_hex_digit(b'A'));
        assert!(AsChar::is_hex_digit(b'F'));
        assert!(!AsChar::is_hex_digit(b'g'));
        assert!(!AsChar::is_hex_digit(b'G'));
        assert!(!AsChar::is_hex_digit(b'/'));
        assert!(!AsChar::is_hex_digit(b':'));
        assert!(!AsChar::is_hex_digit(b'@'));
        assert!(!AsChar::is_hex_digit(b'\x60'));
    }

    #[test]
    fn oct_digit_test() {
        let i = &b"01234567;"[..];
        assert_parse!(
            oct_digit1.parse_peek(Partial::new(i)),
            Ok((Partial::new(&b";"[..]), &i[..i.len() - 1]))
        );

        let i = &b"8"[..];
        assert_parse!(
            oct_digit1.parse_peek(Partial::new(i)),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(i),
                ErrorKind::Slice
            )))
        );

        assert!(AsChar::is_oct_digit(b'0'));
        assert!(AsChar::is_oct_digit(b'7'));
        assert!(!AsChar::is_oct_digit(b'8'));
        assert!(!AsChar::is_oct_digit(b'9'));
        assert!(!AsChar::is_oct_digit(b'a'));
        assert!(!AsChar::is_oct_digit(b'A'));
        assert!(!AsChar::is_oct_digit(b'/'));
        assert!(!AsChar::is_oct_digit(b':'));
        assert!(!AsChar::is_oct_digit(b'@'));
        assert!(!AsChar::is_oct_digit(b'\x60'));
    }

    #[test]
    fn full_line_windows() {
        #[allow(clippy::type_complexity)]
        fn take_full_line(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, (&[u8], &[u8])> {
            (till_line_ending, line_ending).parse_peek(i)
        }
        let input = b"abc\r\n";
        let output = take_full_line(Partial::new(input));
        assert_eq!(
            output,
            Ok((Partial::new(&b""[..]), (&b"abc"[..], &b"\r\n"[..])))
        );
    }

    #[test]
    fn full_line_unix() {
        #[allow(clippy::type_complexity)]
        fn take_full_line(i: Partial<&[u8]>) -> IResult<Partial<&[u8]>, (&[u8], &[u8])> {
            (till_line_ending, line_ending).parse_peek(i)
        }
        let input = b"abc\n";
        let output = take_full_line(Partial::new(input));
        assert_eq!(
            output,
            Ok((Partial::new(&b""[..]), (&b"abc"[..], &b"\n"[..])))
        );
    }

    #[test]
    fn check_windows_lineending() {
        let input = b"\r\n";
        let output = line_ending.parse_peek(Partial::new(&input[..]));
        assert_parse!(output, Ok((Partial::new(&b""[..]), &b"\r\n"[..])));
    }

    #[test]
    fn check_unix_lineending() {
        let input = b"\n";
        let output = line_ending.parse_peek(Partial::new(&input[..]));
        assert_parse!(output, Ok((Partial::new(&b""[..]), &b"\n"[..])));
    }

    #[test]
    fn cr_lf() {
        assert_parse!(
            crlf.parse_peek(Partial::new(&b"\r\na"[..])),
            Ok((Partial::new(&b"a"[..]), &b"\r\n"[..]))
        );
        assert_parse!(
            crlf.parse_peek(Partial::new(&b"\r"[..])),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_parse!(
            crlf.parse_peek(Partial::new(&b"\ra"[..])),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"\ra"[..]),
                ErrorKind::Tag
            )))
        );

        assert_parse!(
            crlf.parse_peek(Partial::new("\r\na")),
            Ok((Partial::new("a"), "\r\n"))
        );
        assert_parse!(
            crlf.parse_peek(Partial::new("\r")),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_parse!(
            crlf.parse_peek(Partial::new("\ra")),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new("\ra"),
                ErrorKind::Tag
            )))
        );
    }

    #[test]
    fn end_of_line() {
        assert_parse!(
            line_ending.parse_peek(Partial::new(&b"\na"[..])),
            Ok((Partial::new(&b"a"[..]), &b"\n"[..]))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new(&b"\r\na"[..])),
            Ok((Partial::new(&b"a"[..]), &b"\r\n"[..]))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new(&b"\r"[..])),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new(&b"\ra"[..])),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"\ra"[..]),
                ErrorKind::Tag
            )))
        );

        assert_parse!(
            line_ending.parse_peek(Partial::new("\na")),
            Ok((Partial::new("a"), "\n"))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new("\r\na")),
            Ok((Partial::new("a"), "\r\n"))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new("\r")),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
        assert_parse!(
            line_ending.parse_peek(Partial::new("\ra")),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new("\ra"),
                ErrorKind::Tag
            )))
        );
    }

    fn digit_to_i16(input: Partial<&str>) -> IResult<Partial<&str>, i16> {
        let i = input;
        let (i, opt_sign) = opt(one_of(['+', '-'])).parse_peek(i)?;
        let sign = match opt_sign {
            Some('+') | None => true,
            Some('-') => false,
            _ => unreachable!(),
        };

        let (i, s) = digit1::<_, InputError<_>>.parse_peek(i)?;
        match s.parse_slice() {
            Some(n) => {
                if sign {
                    Ok((i, n))
                } else {
                    Ok((i, -n))
                }
            }
            None => Err(ErrMode::from_error_kind(&i, ErrorKind::Verify)),
        }
    }

    fn digit_to_u32(i: Partial<&str>) -> IResult<Partial<&str>, u32> {
        let (i, s) = digit1.parse_peek(i)?;
        match s.parse_slice() {
            Some(n) => Ok((i, n)),
            None => Err(ErrMode::from_error_kind(&i, ErrorKind::Verify)),
        }
    }

    proptest! {
      #[test]
      #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
      fn ints(s in "\\PC*") {
          let res1 = digit_to_i16(Partial::new(&s));
          let res2 = dec_int.parse_peek(Partial::new(s.as_str()));
          assert_eq!(res1, res2);
      }

      #[test]
      #[cfg_attr(miri, ignore)]  // See https://github.com/AltSysrq/proptest/issues/253
      fn uints(s in "\\PC*") {
          let res1 = digit_to_u32(Partial::new(&s));
          let res2 = dec_uint.parse_peek(Partial::new(s.as_str()));
          assert_eq!(res1, res2);
      }
    }

    #[test]
    fn hex_uint_tests() {
        fn hex_u32(input: Partial<&[u8]>) -> IResult<Partial<&[u8]>, u32> {
            hex_uint.parse_peek(input)
        }

        assert_parse!(
            hex_u32(Partial::new(&b";"[..])),
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b";"[..]),
                ErrorKind::Slice
            )))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"ff;"[..])),
            Ok((Partial::new(&b";"[..]), 255))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"1be2;"[..])),
            Ok((Partial::new(&b";"[..]), 7138))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"c5a31be2;"[..])),
            Ok((Partial::new(&b";"[..]), 3_315_801_058))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"C5A31be2;"[..])),
            Ok((Partial::new(&b";"[..]), 3_315_801_058))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"00c5a31be2;"[..])), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"00c5a31be2;"[..]),
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"c5a31be201;"[..])), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"c5a31be201;"[..]),
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"ffffffff;"[..])),
            Ok((Partial::new(&b";"[..]), 4_294_967_295))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"ffffffffffffffff;"[..])), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"ffffffffffffffff;"[..]),
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"ffffffffffffffff"[..])), // overflow
            Err(ErrMode::Backtrack(error_position!(
                &Partial::new(&b"ffffffffffffffff"[..]),
                ErrorKind::Verify
            )))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"0x1be2;"[..])),
            Ok((Partial::new(&b"x1be2;"[..]), 0))
        );
        assert_parse!(
            hex_u32(Partial::new(&b"12af"[..])),
            Err(ErrMode::Incomplete(Needed::new(1)))
        );
    }
}
