use snapbox::assert_data_eq;
use snapbox::prelude::*;
use snapbox::str;

#[test]
fn incomplete_inline_table_issue_296() {
    let err = "native = {".parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 11
  |
1 | native = {
  |           ^
invalid inline table
expected `}`

"#]]
        .raw()
    );
}

#[test]
fn bare_value_disallowed_issue_293() {
    let err = "value=zzz".parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | value=zzz
  |       ^
invalid string
expected `"`, `'`

"#]]
        .raw()
    );
}

#[test]
fn bare_value_in_array_disallowed_issue_293() {
    let err = "value=[zzz]".parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | value=[zzz]
  |        ^
invalid array
expected `]`

"#]]
        .raw()
    );
}

#[test]
fn duplicate_table_after_dotted_key_issue_509() {
    let err = "
[dependencies.foo]
version = \"0.16\"

[dependencies]
libc = \"0.2\"

[dependencies]
rand = \"0.3.14\"
"
    .parse::<toml_edit::DocumentMut>()
    .unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 8, column 1
  |
8 | [dependencies]
  | ^
invalid table header
duplicate key `dependencies` in document root

"#]]
        .raw()
    );
}

#[test]
fn bad() {
    let toml_input = "a = 01";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = 01
  |      ^
expected newline, `#`

"#]]
        .raw()
    );

    let toml_input = "a = 1__1";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 1__1
  |       ^
invalid integer
expected digit

"#]]
        .raw()
    );

    let toml_input = "a = 1_";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 1_
  |       ^
invalid integer
expected digit

"#]]
        .raw()
    );

    let toml_input = "''";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ''
  |   ^
expected `.`, `=`

"#]]
        .raw()
    );

    let toml_input = "a = 9e99999";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 5
  |
1 | a = 9e99999
  |     ^
invalid floating-point number

"#]]
        .raw()
    );

    let toml_input = "a = \"\u{7f}\"";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = ""
  |      ^
invalid basic string

"#]]
        .raw()
    );

    let toml_input = "a = '\u{7f}'";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = ''
  |      ^
invalid literal string

"#]]
        .raw()
    );

    let toml_input = "a = -0x1";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = -0x1
  |       ^
expected newline, `#`

"#]]
        .raw()
    );

    let toml_input = "a = 0x-1";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 0x-1
  |       ^
invalid hexadecimal integer

"#]]
        .raw()
    );

    // Dotted keys.
    let toml_input = "a.b.c = 1
         a.b = 2
        ";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 2, column 10
  |
2 |          a.b = 2
  |          ^
duplicate key `b` in document root

"#]]
        .raw()
    );

    let toml_input = "a = 1
         a.b = 2";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 2, column 10
  |
2 |          a.b = 2
  |          ^
dotted key `a` attempted to extend non-table type (integer)

"#]]
        .raw()
    );

    let toml_input = "a = {k1 = 1, k1.name = \"joe\"}";
    let err = toml_input.parse::<toml_edit::DocumentMut>().unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = {k1 = 1, k1.name = "joe"}
  |      ^
dotted key `k1` attempted to extend non-table type (integer)

"#]]
        .raw()
    );
}

#[test]
fn emoji_error_span() {
    let input = "😀";
    let err = input.parse::<toml_edit::DocumentMut>().unwrap_err();
    dbg!(err.span());
    let actual = &input[err.span().unwrap()];
    assert_eq!(actual, input);
}

#[test]
fn text_error_span() {
    let input = "asdf";
    let err = input.parse::<toml_edit::DocumentMut>().unwrap_err();
    dbg!(err.span());
    let actual = &input[err.span().unwrap()];
    assert_eq!(actual, "");
}

#[test]
fn fuzzed_68144_error_span() {
    let input = "\"\\ᾂr\"";
    let err = input.parse::<toml_edit::DocumentMut>().unwrap_err();
    dbg!(err.span());
    let actual = &input[err.span().unwrap()];
    assert_eq!(actual, "ᾂ");
}
