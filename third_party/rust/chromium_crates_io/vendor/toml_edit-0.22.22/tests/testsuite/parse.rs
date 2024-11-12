use snapbox::assert_data_eq;
use snapbox::prelude::*;
use snapbox::str;
use toml_edit::{DocumentMut, Key, Value};

macro_rules! parse {
    ($s:expr, $ty:ty) => {{
        let v = $s.parse::<$ty>();
        assert!(v.is_ok(), "Failed with {}", v.unwrap_err());
        v.unwrap()
    }};
}

macro_rules! parse_value {
    ($s:expr) => {
        parse!($s, Value)
    };
}

macro_rules! test_key {
    ($s:expr, $expected:expr) => {{
        let key = parse!($s, Key);
        assert_eq!($expected, key.get(), "");
    }};
}

#[test]
fn test_key_from_str() {
    test_key!("a", "a");
    test_key!(r#"'hello key'"#, "hello key");
    test_key!(
        r#""Jos\u00E9\U000A0000\n\t\r\f\b\"""#,
        "Jos\u{00E9}\u{A0000}\n\t\r\u{c}\u{8}\""
    );
    test_key!("\"\"", "");
    test_key!("\"'hello key'bla\"", "'hello key'bla");
    test_key!(
        "'C:\\Users\\appveyor\\AppData\\Local\\Temp\\1\\cargo-edit-test.YizxPxxElXn9'",
        "C:\\Users\\appveyor\\AppData\\Local\\Temp\\1\\cargo-edit-test.YizxPxxElXn9"
    );
}

#[test]
fn test_value_from_str() {
    assert!(parse_value!("1979-05-27T00:32:00.999999-07:00").is_datetime());
    assert!(parse_value!("1979-05-27T00:32:00.999999Z").is_datetime());
    assert!(parse_value!("1979-05-27T00:32:00.999999").is_datetime());
    assert!(parse_value!("1979-05-27T00:32:00").is_datetime());
    assert!(parse_value!("1979-05-27").is_datetime());
    assert!(parse_value!("00:32:00").is_datetime());
    assert!(parse_value!("-239").is_integer());
    assert!(parse_value!("1e200").is_float());
    assert!(parse_value!("9_224_617.445_991_228_313").is_float());
    assert!(parse_value!(r#""basic string\nJos\u00E9\n""#).is_str());
    assert!(parse_value!(
        r#""""
multiline basic string
""""#
    )
    .is_str());
    assert!(parse_value!(r"'literal string\ \'").is_str());
    assert!(parse_value!(
        r"'''multiline
literal \ \
string'''"
    )
    .is_str());
    assert!(parse_value!(r#"{ hello = "world", a = 1}"#).is_inline_table());
    assert!(
        parse_value!(r#"[ { x = 1, a = "2" }, {a = "a",b = "b",     c =    "c"} ]"#).is_array()
    );
    let wp = "C:\\Users\\appveyor\\AppData\\Local\\Temp\\1\\cargo-edit-test.YizxPxxElXn9";
    let lwp = "'C:\\Users\\appveyor\\AppData\\Local\\Temp\\1\\cargo-edit-test.YizxPxxElXn9'";
    assert_eq!(Value::from(wp).as_str(), parse_value!(lwp).as_str());
    assert!(parse_value!(r#""\\\"\b\f\n\r\t\u00E9\U000A0000""#).is_str());
}

#[test]
fn test_key_unification() {
    let toml = r#"
[a]
[a.'b'.c]
[a."b".c.e]
[a.b.c.d]
"#;
    let expected = str![[r#"

[a]
[a.'b'.c]
[a.'b'.c.e]
[a.'b'.c.d]

"#]];
    let doc = toml.parse::<DocumentMut>();
    assert!(doc.is_ok());
    let doc = doc.unwrap();

    assert_data_eq!(doc.to_string(), expected.raw());
}

macro_rules! bad {
    ($toml:expr, $msg:expr) => {
        match $toml.parse::<DocumentMut>() {
            Ok(s) => panic!("parsed to: {:#?}", s),
            Err(e) => assert_data_eq!(e.to_string(), $msg.raw()),
        }
    };
}

#[test]
fn crlf() {
    "\
     [project]\r\n\
     \r\n\
     name = \"splay\"\r\n\
     version = \"0.1.0\"\r\n\
     authors = [\"alex@crichton.co\"]\r\n\
     \r\n\
     [[lib]]\r\n\
     \r\n\
     path = \"lib.rs\"\r\n\
     name = \"splay\"\r\n\
     description = \"\"\"\
     A Rust implementation of a TAR file reader and writer. This library does not\r\n\
     currently handle compression, but it is abstract over all I/O readers and\r\n\
     writers. Additionally, great lengths are taken to ensure that the entire\r\n\
     contents are never required to be entirely resident in memory all at once.\r\n\
     \"\"\"\
     "
    .parse::<DocumentMut>()
    .unwrap();
}

#[test]
fn fun_with_strings() {
    let table = r#"
bar = "\U00000000"
key1 = "One\nTwo"
key2 = """One\nTwo"""
key3 = """
One
Two"""

key4 = "The quick brown fox jumps over the lazy dog."
key5 = """
The quick brown \


fox jumps over \
the lazy dog."""
key6 = """\
   The quick brown \
   fox jumps over \
   the lazy dog.\
   """
# What you see is what you get.
winpath  = 'C:\Users\nodejs\templates'
winpath2 = '\\ServerX\admin$\system32\'
quoted   = 'Tom "Dubs" Preston-Werner'
regex    = '<\i\c*\s*>'

regex2 = '''I [dw]on't need \d{2} apples'''
lines  = '''
The first newline is
trimmed in raw strings.
All other whitespace
is preserved.
'''
"#
    .parse::<DocumentMut>()
    .unwrap();
    assert_eq!(table["bar"].as_str(), Some("\0"));
    assert_eq!(table["key1"].as_str(), Some("One\nTwo"));
    assert_eq!(table["key2"].as_str(), Some("One\nTwo"));
    assert_eq!(table["key3"].as_str(), Some("One\nTwo"));

    let msg = "The quick brown fox jumps over the lazy dog.";
    assert_eq!(table["key4"].as_str(), Some(msg));
    assert_eq!(table["key5"].as_str(), Some(msg));
    assert_eq!(table["key6"].as_str(), Some(msg));

    assert_eq!(
        table["winpath"].as_str(),
        Some(r"C:\Users\nodejs\templates")
    );
    assert_eq!(
        table["winpath2"].as_str(),
        Some(r"\\ServerX\admin$\system32\")
    );
    assert_eq!(
        table["quoted"].as_str(),
        Some(r#"Tom "Dubs" Preston-Werner"#)
    );
    assert_eq!(table["regex"].as_str(), Some(r"<\i\c*\s*>"));
    assert_eq!(
        table["regex2"].as_str(),
        Some(r"I [dw]on't need \d{2} apples")
    );
    assert_eq!(
        table["lines"].as_str(),
        Some(
            "The first newline is\n\
             trimmed in raw strings.\n\
             All other whitespace\n\
             is preserved.\n"
        )
    );
}

#[test]
fn tables_in_arrays() {
    let table = r#"
[[foo]]
#…
[foo.bar]
#…

[[foo]] # ...
#…
[foo.bar]
#...
"#
    .parse::<DocumentMut>()
    .unwrap();
    table["foo"][0]["bar"].as_table().unwrap();
    table["foo"][1]["bar"].as_table().unwrap();
}

#[test]
fn empty_table() {
    let table = r#"
[foo]"#
        .parse::<DocumentMut>()
        .unwrap();
    table["foo"].as_table().unwrap();
}

#[test]
fn mixed_table_issue_527() {
    let input = r#"
[package]
metadata.msrv = "1.65.0"

[package.metadata.release.pre-release-replacements]
"#;
    let document = input.parse::<DocumentMut>().unwrap();
    let actual = document.to_string();
    assert_data_eq!(actual, input.raw());
}

#[test]
fn fruit() {
    let table = r#"
[[fruit]]
name = "apple"

[fruit.physical]
color = "red"
shape = "round"

[[fruit.variety]]
name = "red delicious"

[[fruit.variety]]
name = "granny smith"

[[fruit]]
name = "banana"

[[fruit.variety]]
name = "plantain"
"#
    .parse::<DocumentMut>()
    .unwrap();
    assert_eq!(table["fruit"][0]["name"].as_str(), Some("apple"));
    assert_eq!(table["fruit"][0]["physical"]["color"].as_str(), Some("red"));
    assert_eq!(
        table["fruit"][0]["physical"]["shape"].as_str(),
        Some("round")
    );
    assert_eq!(
        table["fruit"][0]["variety"][0]["name"].as_str(),
        Some("red delicious")
    );
    assert_eq!(
        table["fruit"][0]["variety"][1]["name"].as_str(),
        Some("granny smith")
    );
    assert_eq!(table["fruit"][1]["name"].as_str(), Some("banana"));
    assert_eq!(
        table["fruit"][1]["variety"][0]["name"].as_str(),
        Some("plantain")
    );
}

#[test]
fn stray_cr() {
    bad!(
        "\r",
        str![[r#"
TOML parse error at line 1, column 1
  |
1 | 
  | ^


"#]]
    );
    bad!(
        "a = [ \r ]",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | a = [ 
 ]
  |        ^


"#]]
    );
    bad!(
        "a = \"\"\"\r\"\"\"",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | a = """
"""
  |        ^
invalid multiline basic string

"#]]
    );
    bad!(
        "a = \"\"\"\\  \r  \"\"\"",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | a = """\  
  """
  |          ^
invalid escape sequence
expected `b`, `f`, `n`, `r`, `t`, `u`, `U`, `\`, `"`

"#]]
    );
    bad!(
        "a = '''\r'''",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | a = '''
'''
  |        ^
invalid multiline literal string

"#]]
    );
    bad!(
        "a = '\r'",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = '
'
  |      ^
invalid literal string

"#]]
    );
    bad!(
        "a = \"\r\"",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = "
"
  |      ^
invalid basic string

"#]]
    );
}

#[test]
fn blank_literal_string() {
    let table = "foo = ''".parse::<DocumentMut>().unwrap();
    assert_eq!(table["foo"].as_str(), Some(""));
}

#[test]
fn many_blank() {
    let table = "foo = \"\"\"\n\n\n\"\"\"".parse::<DocumentMut>().unwrap();
    assert_eq!(table["foo"].as_str(), Some("\n\n"));
}

#[test]
fn literal_eats_crlf() {
    let table = "
        foo = \"\"\"\\\r\n\"\"\"
        bar = \"\"\"\\\r\n   \r\n   \r\n   a\"\"\"
    "
    .parse::<DocumentMut>()
    .unwrap();
    assert_eq!(table["foo"].as_str(), Some(""));
    assert_eq!(table["bar"].as_str(), Some("a"));
}

#[test]
fn string_no_newline() {
    bad!(
        "a = \"\n\"",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = "
  |      ^
invalid basic string

"#]]
    );
    bad!(
        "a = '\n'",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = '
  |      ^
invalid literal string

"#]]
    );
}

#[test]
fn bad_leading_zeros() {
    bad!(
        "a = 00",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = 00
  |      ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = -00",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = -00
  |       ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = +00",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = +00
  |       ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = 00.0",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = 00.0
  |      ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = -00.0",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = -00.0
  |       ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = +00.0",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = +00.0
  |       ^
expected newline, `#`

"#]]
    );
    bad!(
        "a = 9223372036854775808",
        str![[r#"
TOML parse error at line 1, column 5
  |
1 | a = 9223372036854775808
  |     ^
number too large to fit in target type

"#]]
    );
    bad!(
        "a = -9223372036854775809",
        str![[r#"
TOML parse error at line 1, column 5
  |
1 | a = -9223372036854775809
  |     ^
number too small to fit in target type

"#]]
    );
}

#[test]
fn bad_floats() {
    bad!(
        "a = 0.",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 0.
  |       ^
invalid floating-point number
expected digit

"#]]
    );
    bad!(
        "a = 0.e",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 0.e
  |       ^
invalid floating-point number
expected digit

"#]]
    );
    bad!(
        "a = 0.E",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | a = 0.E
  |       ^
invalid floating-point number
expected digit

"#]]
    );
    bad!(
        "a = 0.0E",
        str![[r#"
TOML parse error at line 1, column 9
  |
1 | a = 0.0E
  |         ^
invalid floating-point number

"#]]
    );
    bad!(
        "a = 0.0e",
        str![[r#"
TOML parse error at line 1, column 9
  |
1 | a = 0.0e
  |         ^
invalid floating-point number

"#]]
    );
    bad!(
        "a = 0.0e-",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | a = 0.0e-
  |          ^
invalid floating-point number

"#]]
    );
    bad!(
        "a = 0.0e+",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | a = 0.0e+
  |          ^
invalid floating-point number

"#]]
    );
}

#[test]
fn floats() {
    macro_rules! t {
        ($actual:expr, $expected:expr) => {{
            let f = format!("foo = {}", $actual);
            println!("{}", f);
            let a = f.parse::<DocumentMut>().unwrap();
            assert_eq!(a["foo"].as_float().unwrap(), $expected);
        }};
    }

    t!("1.0", 1.0);
    t!("1.0e0", 1.0);
    t!("1.0e+0", 1.0);
    t!("1.0e-0", 1.0);
    t!("1E-0", 1.0);
    t!("1.001e-0", 1.001);
    t!("2e10", 2e10);
    t!("2e+10", 2e10);
    t!("2e-10", 2e-10);
    t!("2_0.0", 20.0);
    t!("2_0.0_0e1_0", 20.0e10);
    t!("2_0.1_0e1_0", 20.1e10);
}

#[test]
fn bare_key_names() {
    let a = "
        foo = 3
        foo_3 = 3
        foo_-2--3--r23f--4-f2-4 = 3
        _ = 3
        - = 3
        8 = 8
        \"a\" = 3
        \"!\" = 3
        \"a^b\" = 3
        \"\\\"\" = 3
        \"character encoding\" = \"value\"
        'ʎǝʞ' = \"value\"
    "
    .parse::<DocumentMut>()
    .unwrap();
    let _ = &a["foo"];
    let _ = &a["-"];
    let _ = &a["_"];
    let _ = &a["8"];
    let _ = &a["foo_3"];
    let _ = &a["foo_-2--3--r23f--4-f2-4"];
    let _ = &a["a"];
    let _ = &a["!"];
    let _ = &a["\""];
    let _ = &a["character encoding"];
    let _ = &a["ʎǝʞ"];
}

#[test]
fn bad_keys() {
    bad!(
        "key\n=3",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | key
  |    ^
expected `.`, `=`

"#]]
    );
    bad!(
        "key=\n3",
        str![[r#"
TOML parse error at line 1, column 5
  |
1 | key=
  |     ^
invalid string
expected `"`, `'`

"#]]
    );
    bad!(
        "key|=3",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | key|=3
  |    ^
expected `.`, `=`

"#]]
    );
    bad!(
        "=3",
        str![[r#"
TOML parse error at line 1, column 1
  |
1 | =3
  | ^
invalid key

"#]]
    );
    bad!(
        "\"\"|=3",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ""|=3
  |   ^
expected `.`, `=`

"#]]
    );
    bad!(
        "\"\n\"|=3",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | "
  |  ^
invalid basic string

"#]]
    );
    bad!(
        "\"\r\"|=3",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | "
"|=3
  |  ^
invalid basic string

"#]]
    );
    bad!(
        "''''''=3",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ''''''=3
  |   ^
expected `.`, `=`

"#]]
    );
    bad!(
        "\"\"\"\"\"\"=3",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | """"""=3
  |   ^
expected `.`, `=`

"#]]
    );
    bad!(
        "'''key'''=3",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | '''key'''=3
  |   ^
expected `.`, `=`

"#]]
    );
    bad!(
        "\"\"\"key\"\"\"=3",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | """key"""=3
  |   ^
expected `.`, `=`

"#]]
    );
}

#[test]
fn bad_table_names() {
    bad!(
        "[]",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | []
  |  ^
invalid key

"#]]
    );
    bad!(
        "[.]",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | [.]
  |  ^
invalid key

"#]]
    );
    bad!(
        "[a.]",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | [a.]
  |   ^
invalid table header
expected `.`, `]`

"#]]
    );
    bad!(
        "[!]",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | [!]
  |  ^
invalid key

"#]]
    );
    bad!(
        "[\"\n\"]",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ["
  |   ^
invalid basic string

"#]]
    );
    bad!(
        "[a.b]\n[a.\"b\"]",
        str![[r#"
TOML parse error at line 2, column 1
  |
2 | [a."b"]
  | ^
invalid table header
duplicate key `b` in table `a`

"#]]
    );
    bad!(
        "[']",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | [']
  |    ^
invalid literal string

"#]]
    );
    bad!(
        "[''']",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | [''']
  |    ^
invalid table header
expected `.`, `]`

"#]]
    );
    bad!(
        "['''''']",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | ['''''']
  |    ^
invalid table header
expected `.`, `]`

"#]]
    );
    bad!(
        "['''foo''']",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | ['''foo''']
  |    ^
invalid table header
expected `.`, `]`

"#]]
    );
    bad!(
        "[\"\"\"bar\"\"\"]",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | ["""bar"""]
  |    ^
invalid table header
expected `.`, `]`

"#]]
    );
    bad!(
        "['\n']",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ['
  |   ^
invalid literal string

"#]]
    );
    bad!(
        "['\r\n']",
        str![[r#"
TOML parse error at line 1, column 3
  |
1 | ['
  |   ^
invalid literal string

"#]]
    );
}

#[test]
fn table_names() {
    let a = "
        [a.\"b\"]
        [\"f f\"]
        [\"f.f\"]
        [\"\\\"\"]
        ['a.a']
        ['\"\"']
    "
    .parse::<DocumentMut>()
    .unwrap();
    println!("{:?}", a);
    let _ = &a["a"]["b"];
    let _ = &a["f f"];
    let _ = &a["f.f"];
    let _ = &a["\""];
    let _ = &a["\"\""];
}

#[test]
fn invalid_bare_numeral() {
    bad!(
        "4",
        str![[r#"
TOML parse error at line 1, column 2
  |
1 | 4
  |  ^
expected `.`, `=`

"#]]
    );
}

#[test]
fn inline_tables() {
    "a = {}".parse::<DocumentMut>().unwrap();
    "a = {b=1}".parse::<DocumentMut>().unwrap();
    "a = {   b   =   1    }".parse::<DocumentMut>().unwrap();
    "a = {a=1,b=2}".parse::<DocumentMut>().unwrap();
    "a = {a=1,b=2,c={}}".parse::<DocumentMut>().unwrap();

    bad!(
        "a = {a=1,}",
        str![[r#"
TOML parse error at line 1, column 9
  |
1 | a = {a=1,}
  |         ^
invalid inline table
expected `}`

"#]]
    );
    bad!(
        "a = {,}",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = {,}
  |      ^
invalid inline table
expected `}`

"#]]
    );
    bad!(
        "a = {a=1,a=1}",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = {a=1,a=1}
  |      ^
duplicate key `a`

"#]]
    );
    bad!(
        "a = {\n}",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = {
  |      ^
invalid inline table
expected `}`

"#]]
    );
    bad!(
        "a = {",
        str![[r#"
TOML parse error at line 1, column 6
  |
1 | a = {
  |      ^
invalid inline table
expected `}`

"#]]
    );

    "a = {a=[\n]}".parse::<DocumentMut>().unwrap();
    "a = {\"a\"=[\n]}".parse::<DocumentMut>().unwrap();
    "a = [\n{},\n{},\n]".parse::<DocumentMut>().unwrap();
}

#[test]
fn number_underscores() {
    macro_rules! t {
        ($actual:expr, $expected:expr) => {{
            let f = format!("foo = {}", $actual);
            let table = f.parse::<DocumentMut>().unwrap();
            assert_eq!(table["foo"].as_integer().unwrap(), $expected);
        }};
    }

    t!("1_0", 10);
    t!("1_0_0", 100);
    t!("1_000", 1000);
    t!("+1_000", 1000);
    t!("-1_000", -1000);
}

#[test]
fn bad_underscores() {
    bad!(
        "foo = 0_",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | foo = 0_
  |        ^
expected newline, `#`

"#]]
    );
    bad!(
        "foo = 0__0",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | foo = 0__0
  |        ^
expected newline, `#`

"#]]
    );
    bad!(
        "foo = __0",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | foo = __0
  |       ^
invalid integer
expected leading digit

"#]]
    );
    bad!(
        "foo = 1_0_",
        str![[r#"
TOML parse error at line 1, column 11
  |
1 | foo = 1_0_
  |           ^
invalid integer
expected digit

"#]]
    );
}

#[test]
fn bad_unicode_codepoint() {
    bad!(
        "foo = \"\\uD800\"",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | foo = "\uD800"
  |          ^
invalid unicode 4-digit hex code
value is out of range

"#]]
    );
}

#[test]
fn bad_strings() {
    bad!(
        "foo = \"\\uxx\"",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | foo = "\uxx"
  |          ^
invalid unicode 4-digit hex code

"#]]
    );
    bad!(
        "foo = \"\\u\"",
        str![[r#"
TOML parse error at line 1, column 10
  |
1 | foo = "\u"
  |          ^
invalid unicode 4-digit hex code

"#]]
    );
    bad!(
        "foo = \"\\",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | foo = "\
  |        ^
invalid basic string

"#]]
    );
    bad!(
        "foo = '",
        str![[r#"
TOML parse error at line 1, column 8
  |
1 | foo = '
  |        ^
invalid literal string

"#]]
    );
}

#[test]
fn empty_string() {
    assert_eq!(
        "foo = \"\"".parse::<DocumentMut>().unwrap()["foo"]
            .as_str()
            .unwrap(),
        ""
    );
}

#[test]
fn booleans() {
    let table = "foo = true".parse::<DocumentMut>().unwrap();
    assert_eq!(table["foo"].as_bool(), Some(true));

    let table = "foo = false".parse::<DocumentMut>().unwrap();
    assert_eq!(table["foo"].as_bool(), Some(false));

    bad!(
        "foo = true2",
        str![[r#"
TOML parse error at line 1, column 11
  |
1 | foo = true2
  |           ^
expected newline, `#`

"#]]
    );
    bad!(
        "foo = false2",
        str![[r#"
TOML parse error at line 1, column 12
  |
1 | foo = false2
  |            ^
expected newline, `#`

"#]]
    );
    bad!(
        "foo = t1",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | foo = t1
  |       ^
invalid string
expected `"`, `'`

"#]]
    );
    bad!(
        "foo = f2",
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | foo = f2
  |       ^
invalid string
expected `"`, `'`

"#]]
    );
}

#[test]
fn bad_nesting() {
    bad!(
        "
        a = [2]
        [[a]]
        b = 5
        ",
        str![[r#"
TOML parse error at line 3, column 9
  |
3 |         [[a]]
  |         ^
invalid table header
duplicate key `a` in document root

"#]]
    );
    bad!(
        "
        a = 1
        [a.b]
        ",
        str![[r#"
TOML parse error at line 3, column 9
  |
3 |         [a.b]
  |         ^
invalid table header
dotted key `a` attempted to extend non-table type (integer)

"#]]
    );
    bad!(
        "
        a = []
        [a.b]
        ",
        str![[r#"
TOML parse error at line 3, column 9
  |
3 |         [a.b]
  |         ^
invalid table header
dotted key `a` attempted to extend non-table type (array)

"#]]
    );
    bad!(
        "
        a = []
        [[a.b]]
        ",
        str![[r#"
TOML parse error at line 3, column 9
  |
3 |         [[a.b]]
  |         ^
invalid table header
dotted key `a` attempted to extend non-table type (array)

"#]]
    );
    bad!(
        "
        [a]
        b = { c = 2, d = {} }
        [a.b]
        c = 2
        ",
        str![[r#"
TOML parse error at line 4, column 9
  |
4 |         [a.b]
  |         ^
invalid table header
duplicate key `b` in table `a`

"#]]
    );
}

#[test]
fn bad_table_redefine() {
    bad!(
        "
        [a]
        foo=\"bar\"
        [a.b]
        foo=\"bar\"
        [a]
        ",
        str![[r#"
TOML parse error at line 6, column 9
  |
6 |         [a]
  |         ^
invalid table header
duplicate key `a` in document root

"#]]
    );
    bad!(
        "
        [a]
        foo=\"bar\"
        b = { foo = \"bar\" }
        [a]
        ",
        str![[r#"
TOML parse error at line 5, column 9
  |
5 |         [a]
  |         ^
invalid table header
duplicate key `a` in document root

"#]]
    );
    bad!(
        "
        [a]
        b = {}
        [a.b]
        ",
        str![[r#"
TOML parse error at line 4, column 9
  |
4 |         [a.b]
  |         ^
invalid table header
duplicate key `b` in table `a`

"#]]
    );

    bad!(
        "
        [a]
        b = {}
        [a]
        ",
        str![[r#"
TOML parse error at line 4, column 9
  |
4 |         [a]
  |         ^
invalid table header
duplicate key `a` in document root

"#]]
    );
}

#[test]
fn datetimes() {
    macro_rules! t {
        ($actual:expr) => {{
            let f = format!("foo = {}", $actual);
            let toml = f.parse::<DocumentMut>().expect(&format!("failed: {}", f));
            assert_eq!(toml["foo"].as_datetime().unwrap().to_string(), $actual);
        }};
    }

    t!("2016-09-09T09:09:09Z");
    t!("2016-09-09T09:09:09.1Z");
    t!("2016-09-09T09:09:09.2+10:00");
    t!("2016-09-09T09:09:09.123456789-02:00");
    bad!(
        "foo = 2016-09-09T09:09:09.Z",
        str![[r#"
TOML parse error at line 1, column 26
  |
1 | foo = 2016-09-09T09:09:09.Z
  |                          ^
expected newline, `#`

"#]]
    );
    bad!(
        "foo = 2016-9-09T09:09:09Z",
        str![[r#"
TOML parse error at line 1, column 12
  |
1 | foo = 2016-9-09T09:09:09Z
  |            ^
invalid date-time

"#]]
    );
    bad!(
        "foo = 2016-09-09T09:09:09+2:00",
        str![[r#"
TOML parse error at line 1, column 27
  |
1 | foo = 2016-09-09T09:09:09+2:00
  |                           ^
invalid time offset

"#]]
    );
    bad!(
        "foo = 2016-09-09T09:09:09-2:00",
        str![[r#"
TOML parse error at line 1, column 27
  |
1 | foo = 2016-09-09T09:09:09-2:00
  |                           ^
invalid time offset

"#]]
    );
    bad!(
        "foo = 2016-09-09T09:09:09Z-2:00",
        str![[r#"
TOML parse error at line 1, column 27
  |
1 | foo = 2016-09-09T09:09:09Z-2:00
  |                           ^
expected newline, `#`

"#]]
    );
}

#[test]
fn require_newline_after_value() {
    bad!(
        "0=0r=false",
        str![[r#"
TOML parse error at line 1, column 4
  |
1 | 0=0r=false
  |    ^
expected newline, `#`

"#]]
    );
    bad!(
        r#"
0=""o=""m=""r=""00="0"q="""0"""e="""0"""
"#,
        str![[r#"
TOML parse error at line 2, column 5
  |
2 | 0=""o=""m=""r=""00="0"q="""0"""e="""0"""
  |     ^
expected newline, `#`

"#]]
    );
    bad!(
        r#"
[[0000l0]]
0="0"[[0000l0]]
0="0"[[0000l0]]
0="0"l="0"
"#,
        str![[r#"
TOML parse error at line 3, column 6
  |
3 | 0="0"[[0000l0]]
  |      ^
expected newline, `#`

"#]]
    );
    bad!(
        r#"
0=[0]00=[0,0,0]t=["0","0","0"]s=[1000-00-00T00:00:00Z,2000-00-00T00:00:00Z]
"#,
        str![[r#"
TOML parse error at line 2, column 6
  |
2 | 0=[0]00=[0,0,0]t=["0","0","0"]s=[1000-00-00T00:00:00Z,2000-00-00T00:00:00Z]
  |      ^
expected newline, `#`

"#]]
    );
    bad!(
        r#"
0=0r0=0r=false
"#,
        str![[r#"
TOML parse error at line 2, column 4
  |
2 | 0=0r0=0r=false
  |    ^
expected newline, `#`

"#]]
    );
    bad!(
        r#"
0=0r0=0r=falsefal=false
"#,
        str![[r#"
TOML parse error at line 2, column 4
  |
2 | 0=0r0=0r=falsefal=false
  |    ^
expected newline, `#`

"#]]
    );
}

#[test]
fn dont_use_dotted_key_prefix_on_table_fuzz_57049() {
    // This could generate
    // ```toml
    // [
    // p.o]
    // ```
    let input = r#"
p.a=4
[p.o]
"#;
    let document = input.parse::<DocumentMut>().unwrap();
    let actual = document.to_string();
    assert_data_eq!(actual, input.raw());
}

#[test]
fn despan_keys() {
    let mut doc = r#"aaaaaa = 1"#.parse::<DocumentMut>().unwrap();
    let key = "bbb".parse::<Key>().unwrap();
    let table = doc.as_table_mut();
    table.insert_formatted(
        &key,
        toml_edit::Item::Value(Value::Integer(toml_edit::Formatted::new(2))),
    );

    assert_eq!(doc.to_string(), "aaaaaa = 1\nbbb = 2\n");
}

#[test]
fn dotted_key_comment_roundtrip() {
    let input = r###"
rust.unsafe_op_in_unsafe_fn = "deny"

rust.explicit_outlives_requirements = "warn"
# rust.unused_crate_dependencies = "warn"

clippy.cast_lossless = "warn"
clippy.doc_markdown = "warn"
clippy.exhaustive_enums = "warn"
"###;
    let expected = input;

    let manifest: DocumentMut = input.parse().unwrap();
    let actual = manifest.to_string();

    assert_data_eq!(actual, expected.raw());
}

#[test]
fn string_repr_roundtrip() {
    assert_string_repr_roundtrip(r#""""#, str![[r#""""#]]);
    assert_string_repr_roundtrip(r#""a""#, str![[r#""a""#]]);

    assert_string_repr_roundtrip(r#""tab \t tab""#, str![[r#""tab /t tab""#]]);
    assert_string_repr_roundtrip(r#""lf \n lf""#, str![[r#""lf /n lf""#]]);
    assert_string_repr_roundtrip(r#""crlf \r\n crlf""#, str![[r#""crlf /r/n crlf""#]]);
    assert_string_repr_roundtrip(r#""bell \b bell""#, str![[r#""bell /b bell""#]]);
    assert_string_repr_roundtrip(r#""feed \f feed""#, str![[r#""feed /f feed""#]]);
    assert_string_repr_roundtrip(
        r#""backslash \\ backslash""#,
        str![[r#""backslash // backslash""#]],
    );

    assert_string_repr_roundtrip(r#""squote ' squote""#, str![[r#""squote ' squote""#]]);
    assert_string_repr_roundtrip(
        r#""triple squote ''' triple squote""#,
        str![[r#""triple squote ''' triple squote""#]],
    );
    assert_string_repr_roundtrip(r#""end squote '""#, str![[r#""end squote '""#]]);

    assert_string_repr_roundtrip(r#""quote \" quote""#, str![[r#""quote /" quote""#]]);
    assert_string_repr_roundtrip(
        r#""triple quote \"\"\" triple quote""#,
        str![[r#""triple quote /"/"/" triple quote""#]],
    );
    assert_string_repr_roundtrip(r#""end quote \"""#, str![[r#""end quote /"""#]]);
    assert_string_repr_roundtrip(
        r#""quoted \"content\" quoted""#,
        str![[r#""quoted /"content/" quoted""#]],
    );
    assert_string_repr_roundtrip(
        r#""squoted 'content' squoted""#,
        str![[r#""squoted 'content' squoted""#]],
    );
    assert_string_repr_roundtrip(
        r#""mixed quoted \"start\" 'end'' mixed quote""#,
        str![[r#""mixed quoted /"start/" 'end'' mixed quote""#]],
    );
}

#[track_caller]
fn assert_string_repr_roundtrip(input: &str, expected: impl IntoData) {
    let value: Value = input.parse().unwrap();
    let actual = value.to_string();
    let _: Value = actual.parse().unwrap_or_else(|_err| {
        panic!(
            "invalid `Value`:
```
{actual}
```
"
        )
    });
    let expected = expected.into_data();
    assert_data_eq!(actual, expected);
}

#[test]
fn string_value_roundtrip() {
    assert_string_value_roundtrip(r#""""#, str![[r#""""#]]);
    assert_string_value_roundtrip(r#""a""#, str![[r#""a""#]]);

    assert_string_value_roundtrip(r#""tab \t tab""#, str![[r#""tab /t tab""#]]);
    assert_string_value_roundtrip(
        r#""lf \n lf""#,
        str![[r#"
"""
lf 
 lf"""
"#]],
    );
    assert_string_value_roundtrip(
        r#""crlf \r\n crlf""#,
        str![[r#"
"""
crlf /r
 crlf"""
"#]],
    );
    assert_string_value_roundtrip(r#""bell \b bell""#, str![[r#""bell /b bell""#]]);
    assert_string_value_roundtrip(r#""feed \f feed""#, str![[r#""feed /f feed""#]]);
    assert_string_value_roundtrip(
        r#""backslash \\ backslash""#,
        str!["'backslash / backslash'"],
    );

    assert_string_value_roundtrip(r#""squote ' squote""#, str![[r#""squote ' squote""#]]);
    assert_string_value_roundtrip(
        r#""triple squote ''' triple squote""#,
        str![[r#""triple squote ''' triple squote""#]],
    );
    assert_string_value_roundtrip(r#""end squote '""#, str![[r#""end squote '""#]]);

    assert_string_value_roundtrip(r#""quote \" quote""#, str![[r#"'quote " quote'"#]]);
    assert_string_value_roundtrip(
        r#""triple quote \"\"\" triple quote""#,
        str![[r#"'triple quote """ triple quote'"#]],
    );
    assert_string_value_roundtrip(r#""end quote \"""#, str![[r#"'end quote "'"#]]);
    assert_string_value_roundtrip(
        r#""quoted \"content\" quoted""#,
        str![[r#"'quoted "content" quoted'"#]],
    );
    assert_string_value_roundtrip(
        r#""squoted 'content' squoted""#,
        str![[r#""squoted 'content' squoted""#]],
    );
    assert_string_value_roundtrip(
        r#""mixed quoted \"start\" 'end'' mixed quote""#,
        str![[r#"'''mixed quoted "start" 'end'' mixed quote'''"#]],
    );
}

#[track_caller]
fn assert_string_value_roundtrip(input: &str, expected: impl IntoData) {
    let value: Value = input.parse().unwrap();
    let value = Value::from(value.as_str().unwrap()); // Remove repr
    let actual = value.to_string();
    let _: Value = actual.parse().unwrap_or_else(|_err| {
        panic!(
            "invalid `Value`:
```
{actual}
```
"
        )
    });
    let expected = expected.into_data();
    assert_data_eq!(actual, expected);
}

#[test]
fn key_repr_roundtrip() {
    assert_key_repr_roundtrip(r#""""#, str![[r#""""#]]);
    assert_key_repr_roundtrip(r#""a""#, str![[r#""a""#]]);

    assert_key_repr_roundtrip(r#""tab \t tab""#, str![[r#""tab /t tab""#]]);
    assert_key_repr_roundtrip(r#""lf \n lf""#, str![[r#""lf /n lf""#]]);
    assert_key_repr_roundtrip(r#""crlf \r\n crlf""#, str![[r#""crlf /r/n crlf""#]]);
    assert_key_repr_roundtrip(r#""bell \b bell""#, str![[r#""bell /b bell""#]]);
    assert_key_repr_roundtrip(r#""feed \f feed""#, str![[r#""feed /f feed""#]]);
    assert_key_repr_roundtrip(
        r#""backslash \\ backslash""#,
        str![[r#""backslash // backslash""#]],
    );

    assert_key_repr_roundtrip(r#""squote ' squote""#, str![[r#""squote ' squote""#]]);
    assert_key_repr_roundtrip(
        r#""triple squote ''' triple squote""#,
        str![[r#""triple squote ''' triple squote""#]],
    );
    assert_key_repr_roundtrip(r#""end squote '""#, str![[r#""end squote '""#]]);

    assert_key_repr_roundtrip(r#""quote \" quote""#, str![[r#""quote /" quote""#]]);
    assert_key_repr_roundtrip(
        r#""triple quote \"\"\" triple quote""#,
        str![[r#""triple quote /"/"/" triple quote""#]],
    );
    assert_key_repr_roundtrip(r#""end quote \"""#, str![[r#""end quote /"""#]]);
    assert_key_repr_roundtrip(
        r#""quoted \"content\" quoted""#,
        str![[r#""quoted /"content/" quoted""#]],
    );
    assert_key_repr_roundtrip(
        r#""squoted 'content' squoted""#,
        str![[r#""squoted 'content' squoted""#]],
    );
    assert_key_repr_roundtrip(
        r#""mixed quoted \"start\" 'end'' mixed quote""#,
        str![[r#""mixed quoted /"start/" 'end'' mixed quote""#]],
    );
}

#[track_caller]
fn assert_key_repr_roundtrip(input: &str, expected: impl IntoData) {
    let value: Key = input.parse().unwrap();
    let actual = value.to_string();
    let _: Key = actual.parse().unwrap_or_else(|_err| {
        panic!(
            "invalid `Key`:
```
{actual}
```
"
        )
    });
    let expected = expected.into_data();
    assert_data_eq!(actual, expected);
}

#[test]
fn key_value_roundtrip() {
    assert_key_value_roundtrip(r#""""#, str![[r#""""#]]);
    assert_key_value_roundtrip(r#""a""#, str!["a"]);

    assert_key_value_roundtrip(r#""tab \t tab""#, str![[r#""tab /t tab""#]]);
    assert_key_value_roundtrip(r#""lf \n lf""#, str![[r#""lf /n lf""#]]);
    assert_key_value_roundtrip(r#""crlf \r\n crlf""#, str![[r#""crlf /r/n crlf""#]]);
    assert_key_value_roundtrip(r#""bell \b bell""#, str![[r#""bell /b bell""#]]);
    assert_key_value_roundtrip(r#""feed \f feed""#, str![[r#""feed /f feed""#]]);
    assert_key_value_roundtrip(
        r#""backslash \\ backslash""#,
        str!["'backslash / backslash'"],
    );

    assert_key_value_roundtrip(r#""squote ' squote""#, str![[r#""squote ' squote""#]]);
    assert_key_value_roundtrip(
        r#""triple squote ''' triple squote""#,
        str![[r#""triple squote ''' triple squote""#]],
    );
    assert_key_value_roundtrip(r#""end squote '""#, str![[r#""end squote '""#]]);

    assert_key_value_roundtrip(r#""quote \" quote""#, str![[r#"'quote " quote'"#]]);
    assert_key_value_roundtrip(
        r#""triple quote \"\"\" triple quote""#,
        str![[r#"'triple quote """ triple quote'"#]],
    );
    assert_key_value_roundtrip(r#""end quote \"""#, str![[r#"'end quote "'"#]]);
    assert_key_value_roundtrip(
        r#""quoted \"content\" quoted""#,
        str![[r#"'quoted "content" quoted'"#]],
    );
    assert_key_value_roundtrip(
        r#""squoted 'content' squoted""#,
        str![[r#""squoted 'content' squoted""#]],
    );
    assert_key_value_roundtrip(
        r#""mixed quoted \"start\" 'end'' mixed quote""#,
        str![[r#""mixed quoted /"start/" 'end'' mixed quote""#]],
    );
}

#[track_caller]
fn assert_key_value_roundtrip(input: &str, expected: impl IntoData) {
    let value: Key = input.parse().unwrap();
    let value = Key::new(value.get()); // Remove repr
    let actual = value.to_string();
    let _: Key = actual.parse().unwrap_or_else(|_err| {
        panic!(
            "invalid `Key`:
```
{actual}
```
"
        )
    });
    let expected = expected.into_data();
    assert_data_eq!(actual, expected);
}
