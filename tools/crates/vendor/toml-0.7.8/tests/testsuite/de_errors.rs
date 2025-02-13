use serde::{de, Deserialize};
use std::fmt;

macro_rules! bad {
    ($toml:expr, $ty:ty, $msg:expr) => {
        match toml::from_str::<$ty>($toml) {
            Ok(s) => panic!("parsed to: {:#?}", s),
            Err(e) => snapbox::assert_eq($msg, e.to_string()),
        }
    };
}

#[derive(Debug, Deserialize, PartialEq)]
struct Parent<T> {
    p_a: T,
    p_b: Vec<Child<T>>,
}

#[derive(Debug, Deserialize, PartialEq)]
#[serde(deny_unknown_fields)]
struct Child<T> {
    c_a: T,
    c_b: T,
}

#[derive(Debug, PartialEq)]
enum CasedString {
    Lowercase(String),
    Uppercase(String),
}

impl<'de> de::Deserialize<'de> for CasedString {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct CasedStringVisitor;

        impl<'de> de::Visitor<'de> for CasedStringVisitor {
            type Value = CasedString;

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                formatter.write_str("a string")
            }

            fn visit_str<E>(self, s: &str) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                if s.is_empty() {
                    Err(de::Error::invalid_length(0, &"a non-empty string"))
                } else if s.chars().all(|x| x.is_ascii_lowercase()) {
                    Ok(CasedString::Lowercase(s.to_string()))
                } else if s.chars().all(|x| x.is_ascii_uppercase()) {
                    Ok(CasedString::Uppercase(s.to_string()))
                } else {
                    Err(de::Error::invalid_value(
                        de::Unexpected::Str(s),
                        &"all lowercase or all uppercase",
                    ))
                }
            }
        }

        deserializer.deserialize_any(CasedStringVisitor)
    }
}

#[test]
fn custom_errors() {
    toml::from_str::<Parent<CasedString>>(
        "
            p_a = 'a'
            p_b = [{c_a = 'a', c_b = 'c'}]
        ",
    )
    .unwrap();

    // Custom error at p_b value.
    bad!(
        "
            p_a = ''
                # ^
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 2, column 19
  |
2 |             p_a = ''
  |                   ^^
invalid length 0, expected a non-empty string
"
    );

    // Missing field in table.
    bad!(
        "
            p_a = 'a'
          # ^
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 1, column 1
  |
1 | 
  | ^
missing field `p_b`
"
    );

    // Invalid type in p_b.
    bad!(
        "
            p_a = 'a'
            p_b = 1
                # ^
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 3, column 19
  |
3 |             p_b = 1
  |                   ^
invalid type: integer `1`, expected a sequence
"
    );

    // Sub-table in Vec is missing a field.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a'}
              # ^
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 4, column 17
  |
4 |                 {c_a = 'a'}
  |                 ^^^^^^^^^^^
missing field `c_b`
"
    );

    // Sub-table in Vec has a field with a bad value.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a', c_b = '*'}
                                # ^
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 4, column 35
  |
4 |                 {c_a = 'a', c_b = '*'}
  |                                   ^^^
invalid value: string \"*\", expected all lowercase or all uppercase
"
    );

    // Sub-table in Vec is missing a field.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a', c_b = 'b'},
                {c_a = 'aa'}
              # ^
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 5, column 17
  |
5 |                 {c_a = 'aa'}
  |                 ^^^^^^^^^^^^
missing field `c_b`
"
    );

    // Sub-table in the middle of a Vec is missing a field.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a', c_b = 'b'},
                {c_a = 'aa'},
              # ^
                {c_a = 'aaa', c_b = 'bbb'},
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 5, column 17
  |
5 |                 {c_a = 'aa'},
  |                 ^^^^^^^^^^^^
missing field `c_b`
"
    );

    // Sub-table in the middle of a Vec has a field with a bad value.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a', c_b = 'b'},
                {c_a = 'aa', c_b = 1},
                                 # ^
                {c_a = 'aaa', c_b = 'bbb'},
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 5, column 36
  |
5 |                 {c_a = 'aa', c_b = 1},
  |                                    ^
invalid type: integer `1`, expected a string
"
    );

    // Sub-table in the middle of a Vec has an extra field.
    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = 'a', c_b = 'b'},
                {c_a = 'aa', c_b = 'bb', c_d = 'd'},
              # ^
                {c_a = 'aaa', c_b = 'bbb'},
                {c_a = 'aaaa', c_b = 'bbbb'},
            ]
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 5, column 42
  |
5 |                 {c_a = 'aa', c_b = 'bb', c_d = 'd'},
  |                                          ^^^
unknown field `c_d`, expected `c_a` or `c_b`
"
    );

    // Sub-table in the middle of a Vec is missing a field.
    // FIXME: This location is pretty off.
    bad!(
        "
            p_a = 'a'
            [[p_b]]
            c_a = 'a'
            c_b = 'b'
            [[p_b]]
            c_a = 'aa'
            # c_b = 'bb' # <- missing field
            [[p_b]]
            c_a = 'aaa'
            c_b = 'bbb'
            [[p_b]]
          # ^
            c_a = 'aaaa'
            c_b = 'bbbb'
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 6, column 13
  |
6 |             [[p_b]]
  |             ^^^^^^^^^^^^^^^^^^^
missing field `c_b`
"
    );

    // Sub-table in the middle of a Vec has a field with a bad value.
    bad!(
        "
            p_a = 'a'
            [[p_b]]
            c_a = 'a'
            c_b = 'b'
            [[p_b]]
            c_a = 'aa'
            c_b = '*'
                # ^
            [[p_b]]
            c_a = 'aaa'
            c_b = 'bbb'
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 8, column 19
  |
8 |             c_b = '*'
  |                   ^^^
invalid value: string \"*\", expected all lowercase or all uppercase
"
    );

    // Sub-table in the middle of a Vec has an extra field.
    bad!(
        "
            p_a = 'a'
            [[p_b]]
            c_a = 'a'
            c_b = 'b'
            [[p_b]]
            c_a = 'aa'
            c_d = 'dd' # unknown field
          # ^
            [[p_b]]
            c_a = 'aaa'
            c_b = 'bbb'
            [[p_b]]
            c_a = 'aaaa'
            c_b = 'bbbb'
        ",
        Parent<CasedString>,
        "\
TOML parse error at line 8, column 13
  |
8 |             c_d = 'dd' # unknown field
  |             ^^^
unknown field `c_d`, expected `c_a` or `c_b`
"
    );
}

#[test]
fn serde_derive_deserialize_errors() {
    bad!(
        "
            p_a = ''
          # ^
        ",
        Parent<String>,
        "\
TOML parse error at line 1, column 1
  |
1 | 
  | ^
missing field `p_b`
"
    );

    bad!(
        "
            p_a = ''
            p_b = [
                {c_a = ''}
              # ^
            ]
        ",
        Parent<String>,
        "\
TOML parse error at line 4, column 17
  |
4 |                 {c_a = ''}
  |                 ^^^^^^^^^^
missing field `c_b`
"
    );

    bad!(
        "
            p_a = ''
            p_b = [
                {c_a = '', c_b = 1}
                               # ^
            ]
        ",
        Parent<String>,
        "\
TOML parse error at line 4, column 34
  |
4 |                 {c_a = '', c_b = 1}
  |                                  ^
invalid type: integer `1`, expected a string
"
    );

    // FIXME: This location could be better.
    bad!(
        "
            p_a = ''
            p_b = [
                {c_a = '', c_b = '', c_d = ''},
              # ^
            ]
        ",
        Parent<String>,
        "\
TOML parse error at line 4, column 38
  |
4 |                 {c_a = '', c_b = '', c_d = ''},
  |                                      ^^^
unknown field `c_d`, expected `c_a` or `c_b`
"
    );

    bad!(
        "
            p_a = 'a'
            p_b = [
                {c_a = '', c_b = 1, c_d = ''},
                               # ^
            ]
        ",
        Parent<String>,
        "\
TOML parse error at line 4, column 34
  |
4 |                 {c_a = '', c_b = 1, c_d = ''},
  |                                  ^
invalid type: integer `1`, expected a string
"
    );
}

#[test]
fn error_handles_crlf() {
    bad!(
        "\r\n\
         [t1]\r\n\
         [t2]\r\n\
         a = 1\r\n\
         a = 2\r\n\
         ",
        toml::Value,
        "\
TOML parse error at line 5, column 1
  |
5 | a = 2
  | ^
duplicate key `a` in table `t2`
"
    );

    // Should be the same as above.
    bad!(
        "\n\
         [t1]\n\
         [t2]\n\
         a = 1\n\
         a = 2\n\
         ",
        toml::Value,
        "\
TOML parse error at line 5, column 1
  |
5 | a = 2
  | ^
duplicate key `a` in table `t2`
"
    );
}
