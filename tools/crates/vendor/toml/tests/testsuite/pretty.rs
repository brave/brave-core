use serde::ser::Serialize;
use snapbox::assert_data_eq;
use snapbox::prelude::*;

const NO_PRETTY: &str = "\
[example]
array = [\"item 1\", \"item 2\"]
empty = []
oneline = \"this has no newlines.\"
text = '''

this is the first line\\nthis is the second line
'''
";

#[test]
fn no_pretty() {
    let toml = NO_PRETTY;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value.serialize(toml::Serializer::new(&mut result)).unwrap();
    assert_data_eq!(&result, toml.raw());
}

const PRETTY_STD: &str = "\
[example]
array = [
    \"item 1\",
    \"item 2\",
]
empty = []
one = [\"one\"]
oneline = \"this has no newlines.\"
text = \"\"\"
this is the first line
this is the second line
\"\"\"
";

#[test]
fn pretty_std() {
    let toml = PRETTY_STD;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value
        .serialize(toml::Serializer::pretty(&mut result))
        .unwrap();
    assert_data_eq!(&result, toml.raw());
}

const PRETTY_TRICKY: &str = r#"[example]
f = "\f"
glass = """
Nothing too unusual, except that I can eat glass in:
- Greek: Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα. 
- Polish: Mogę jeść szkło, i mi nie szkodzi. 
- Hindi: मैं काँच खा सकता हूँ, मुझे उस से कोई पीडा नहीं होती. 
- Japanese: 私はガラスを食べられます。それは私を傷つけません。 
"""
r = "\r"
r_newline = """
\r
"""
single = "this is a single line but has '' cuz it's tricky"
single_tricky = "single line with ''' in it"
tabs = """
this is pretty standard
\texcept for some   \ttabs right here
"""
text = """
this is the first line.
This has a ''' in it and \"\"\" cuz it's tricky yo
Also ' and \" because why not
this is the fourth line
"""
"#;

#[test]
fn pretty_tricky() {
    let toml = PRETTY_TRICKY;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value
        .serialize(toml::Serializer::pretty(&mut result))
        .unwrap();
    assert_data_eq!(&result, toml.raw());
}

const PRETTY_TABLE_ARRAY: &str = r#"[[array]]
key = "foo"

[[array]]
key = "bar"

[abc]
doc = "this is a table"

[example]
single = "this is a single line string"
"#;

#[test]
fn pretty_table_array() {
    let toml = PRETTY_TABLE_ARRAY;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value
        .serialize(toml::Serializer::pretty(&mut result))
        .unwrap();
    assert_data_eq!(&result, toml.raw());
}

const TABLE_ARRAY: &str = r#"[[array]]
key = "foo"

[[array]]
key = "bar"

[abc]
doc = "this is a table"

[example]
single = "this is a single line string"
"#;

#[test]
fn table_array() {
    let toml = TABLE_ARRAY;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value.serialize(toml::Serializer::new(&mut result)).unwrap();
    assert_data_eq!(&result, toml.raw());
}

const PRETTY_EMPTY_TABLE: &str = r#"[example]
"#;

#[test]
fn pretty_empty_table() {
    let toml = PRETTY_EMPTY_TABLE;
    let value: toml::Value = toml::from_str(toml).unwrap();
    let mut result = String::with_capacity(128);
    value.serialize(toml::Serializer::new(&mut result)).unwrap();
    assert_data_eq!(&result, toml.raw());
}

#[test]
fn error_includes_key() {
    #[derive(Debug, serde::Serialize, serde::Deserialize)]
    struct Package {
        name: String,
        version: String,
        authors: Vec<String>,
        profile: Profile,
    }

    #[derive(Debug, serde::Serialize, serde::Deserialize)]
    struct Profile {
        dev: Dev,
    }

    #[derive(Debug, serde::Serialize, serde::Deserialize)]
    struct Dev {
        debug: U32OrBool,
    }

    #[derive(Clone, Debug, serde::Deserialize, serde::Serialize, Eq, PartialEq)]
    #[serde(untagged, expecting = "expected a boolean or an integer")]
    pub(crate) enum U32OrBool {
        U32(u32),
        Bool(bool),
    }

    let raw = r#"name = "foo"
version = "0.0.0"
authors = []

[profile.dev]
debug = true
"#;

    let pkg: Package = toml::from_str(raw).unwrap();
    let pretty = toml::to_string_pretty(&pkg).unwrap();
    assert_data_eq!(pretty, raw.raw());
}
