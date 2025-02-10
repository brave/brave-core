macro_rules! bad {
    ($toml:expr, $msg:expr) => {
        match $toml.parse::<toml_edit::Document>() {
            Ok(s) => panic!("parsed to: {:#?}", s),
            Err(e) => snapbox::assert_eq($msg, e.to_string()),
        }
    };
}

#[test]
fn times() {
    fn dogood(s: &str, serialized: &str) {
        let to_parse = format!("foo = {}", s);
        let document = to_parse.parse::<toml_edit::Document>().unwrap();
        assert_eq!(
            document["foo"].as_datetime().unwrap().to_string(),
            serialized
        );
    }
    fn good(s: &str) {
        dogood(s, s);
        dogood(&s.replace('T', " "), s);
        dogood(&s.replace('T', "t"), s);
        dogood(&s.replace('Z', "z"), s);
    }

    good("1997-09-09T09:09:09Z");
    good("1997-09-09T09:09:09+09:09");
    good("1997-09-09T09:09:09-09:09");
    good("1997-09-09T09:09:09");
    good("1997-09-09");
    dogood("1997-09-09 ", "1997-09-09");
    dogood("1997-09-09 # comment", "1997-09-09");
    good("09:09:09");
    good("1997-09-09T09:09:09.09Z");
    good("1997-09-09T09:09:09.09+09:09");
    good("1997-09-09T09:09:09.09-09:09");
    good("1997-09-09T09:09:09.09");
    good("09:09:09.09");
}

#[test]
fn bad_times() {
    bad!(
        "foo = 199-09-09",
        "\
TOML parse error at line 1, column 10
  |
1 | foo = 199-09-09
  |          ^
expected newline, `#`
"
    );
    bad!(
        "foo = 199709-09",
        "\
TOML parse error at line 1, column 13
  |
1 | foo = 199709-09
  |             ^
expected newline, `#`
"
    );
    bad!(
        "foo = 1997-9-09",
        "\
TOML parse error at line 1, column 12
  |
1 | foo = 1997-9-09
  |            ^
invalid date-time
"
    );
    bad!(
        "foo = 1997-09-9",
        "\
TOML parse error at line 1, column 15
  |
1 | foo = 1997-09-9
  |               ^
invalid date-time
"
    );
    bad!(
        "foo = 1997-09-0909:09:09",
        "\
TOML parse error at line 1, column 17
  |
1 | foo = 1997-09-0909:09:09
  |                 ^
expected newline, `#`
"
    );
    bad!(
        "foo = 1997-09-09T09:09:09.",
        "\
TOML parse error at line 1, column 26
  |
1 | foo = 1997-09-09T09:09:09.
  |                          ^
expected newline, `#`
"
    );
    bad!(
        "foo = T",
        r#"TOML parse error at line 1, column 7
  |
1 | foo = T
  |       ^
invalid string
expected `"`, `'`
"#
    );
    bad!(
        "foo = T.",
        r#"TOML parse error at line 1, column 7
  |
1 | foo = T.
  |       ^
invalid string
expected `"`, `'`
"#
    );
    bad!(
        "foo = TZ",
        r#"TOML parse error at line 1, column 7
  |
1 | foo = TZ
  |       ^
invalid string
expected `"`, `'`
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09+",
        r#"TOML parse error at line 1, column 30
  |
1 | foo = 1997-09-09T09:09:09.09+
  |                              ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09+09",
        r#"TOML parse error at line 1, column 32
  |
1 | foo = 1997-09-09T09:09:09.09+09
  |                                ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09+09:9",
        r#"TOML parse error at line 1, column 33
  |
1 | foo = 1997-09-09T09:09:09.09+09:9
  |                                 ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09+0909",
        r#"TOML parse error at line 1, column 32
  |
1 | foo = 1997-09-09T09:09:09.09+0909
  |                                ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09-",
        r#"TOML parse error at line 1, column 30
  |
1 | foo = 1997-09-09T09:09:09.09-
  |                              ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09-09",
        r#"TOML parse error at line 1, column 32
  |
1 | foo = 1997-09-09T09:09:09.09-09
  |                                ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09-09:9",
        r#"TOML parse error at line 1, column 33
  |
1 | foo = 1997-09-09T09:09:09.09-09:9
  |                                 ^
invalid time offset
"#
    );
    bad!(
        "foo = 1997-09-09T09:09:09.09-0909",
        r#"TOML parse error at line 1, column 32
  |
1 | foo = 1997-09-09T09:09:09.09-0909
  |                                ^
invalid time offset
"#
    );

    bad!(
        "foo = 1997-00-09T09:09:09.09Z",
        r#"TOML parse error at line 1, column 12
  |
1 | foo = 1997-00-09T09:09:09.09Z
  |            ^
invalid date-time
value is out of range
"#
    );
    bad!(
        "foo = 1997-09-00T09:09:09.09Z",
        r#"TOML parse error at line 1, column 15
  |
1 | foo = 1997-09-00T09:09:09.09Z
  |               ^
invalid date-time
value is out of range
"#
    );
    bad!(
        "foo = 1997-09-09T30:09:09.09Z",
        r#"TOML parse error at line 1, column 17
  |
1 | foo = 1997-09-09T30:09:09.09Z
  |                 ^
expected newline, `#`
"#
    );
    bad!(
        "foo = 1997-09-09T12:69:09.09Z",
        r#"TOML parse error at line 1, column 21
  |
1 | foo = 1997-09-09T12:69:09.09Z
  |                     ^
invalid date-time
value is out of range
"#
    );
    bad!(
        "foo = 1997-09-09T12:09:69.09Z",
        r#"TOML parse error at line 1, column 24
  |
1 | foo = 1997-09-09T12:09:69.09Z
  |                        ^
invalid date-time
value is out of range
"#
    );
}
