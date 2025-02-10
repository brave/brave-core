#[test]
fn incomplete_inline_table_issue_296() {
    let err = "native = {".parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(
        r#"TOML parse error at line 1, column 11
  |
1 | native = {
  |           ^
invalid inline table
expected `}`
"#,
        err.to_string(),
    );
}

#[test]
fn bare_value_disallowed_issue_293() {
    let err = "value=zzz".parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(
        r#"TOML parse error at line 1, column 7
  |
1 | value=zzz
  |       ^
invalid string
expected `"`, `'`
"#,
        err.to_string(),
    );
}

#[test]
fn bare_value_in_array_disallowed_issue_293() {
    let err = "value=[zzz]".parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(
        r#"TOML parse error at line 1, column 8
  |
1 | value=[zzz]
  |        ^
invalid array
expected `]`
"#,
        err.to_string(),
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
    .parse::<toml_edit::Document>()
    .unwrap_err();
    snapbox::assert_eq(
        r#"TOML parse error at line 8, column 1
  |
8 | [dependencies]
  | ^
invalid table header
duplicate key `dependencies` in document root
"#,
        err.to_string(),
    );
}

#[test]
fn bad() {
    let toml_input = "a = 01";
    let expected_err = "\
TOML parse error at line 1, column 6
  |
1 | a = 01
  |      ^
expected newline, `#`
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = 1__1";
    let expected_err = "\
TOML parse error at line 1, column 7
  |
1 | a = 1__1
  |       ^
invalid integer
expected digit
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = 1_";
    let expected_err = "\
TOML parse error at line 1, column 7
  |
1 | a = 1_
  |       ^
invalid integer
expected digit
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "''";
    let expected_err = "\
TOML parse error at line 1, column 3
  |
1 | ''
  |   ^
expected `.`, `=`
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = 9e99999";
    let expected_err = "\
TOML parse error at line 1, column 5
  |
1 | a = 9e99999
  |     ^
invalid floating-point number
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = \"\u{7f}\"";
    let expected_err = "\
TOML parse error at line 1, column 6
  |
1 | a = \"\u{7f}\"
  |      ^
invalid basic string
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = '\u{7f}'";
    let expected_err = "\
TOML parse error at line 1, column 6
  |
1 | a = '\u{7f}'
  |      ^
invalid literal string
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = -0x1";
    let expected_err = "\
TOML parse error at line 1, column 7
  |
1 | a = -0x1
  |       ^
expected newline, `#`
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = 0x-1";
    let expected_err = "\
TOML parse error at line 1, column 7
  |
1 | a = 0x-1
  |       ^
invalid hexadecimal integer
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    // Dotted keys.
    let toml_input = "a.b.c = 1
         a.b = 2
        ";
    let expected_err = "\
TOML parse error at line 2, column 10
  |
2 |          a.b = 2
  |          ^
duplicate key `b` in document root
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = 1
         a.b = 2";
    let expected_err = "\
TOML parse error at line 2, column 10
  |
2 |          a.b = 2
  |          ^
dotted key `a` attempted to extend non-table type (integer)
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());

    let toml_input = "a = {k1 = 1, k1.name = \"joe\"}";
    let expected_err = "\
TOML parse error at line 1, column 6
  |
1 | a = {k1 = 1, k1.name = \"joe\"}
  |      ^
dotted key `k1` attempted to extend non-table type (integer)
";
    let err = toml_input.parse::<toml_edit::Document>().unwrap_err();
    snapbox::assert_eq(expected_err, err.to_string());
}
