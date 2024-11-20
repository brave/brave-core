use std::borrow::Cow;
use std::fmt::{Display, Formatter, Result, Write};

use toml_datetime::Datetime;

use crate::inline_table::DEFAULT_INLINE_KEY_DECOR;
use crate::key::Key;
use crate::repr::{Formatted, Repr, ValueRepr};
use crate::table::{
    DEFAULT_KEY_DECOR, DEFAULT_KEY_PATH_DECOR, DEFAULT_ROOT_DECOR, DEFAULT_TABLE_DECOR,
};
use crate::value::{
    DEFAULT_LEADING_VALUE_DECOR, DEFAULT_TRAILING_VALUE_DECOR, DEFAULT_VALUE_DECOR,
};
use crate::DocumentMut;
use crate::{Array, InlineTable, Item, Table, Value};

pub(crate) fn encode_key(this: &Key, buf: &mut dyn Write, input: Option<&str>) -> Result {
    if let Some(input) = input {
        let repr = this
            .as_repr()
            .map(Cow::Borrowed)
            .unwrap_or_else(|| Cow::Owned(this.default_repr()));
        repr.encode(buf, input)?;
    } else {
        let repr = this.display_repr();
        write!(buf, "{}", repr)?;
    };

    Ok(())
}

fn encode_key_path(
    this: &[Key],
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    let leaf_decor = this.last().expect("always at least one key").leaf_decor();
    for (i, key) in this.iter().enumerate() {
        let dotted_decor = key.dotted_decor();

        let first = i == 0;
        let last = i + 1 == this.len();

        if first {
            leaf_decor.prefix_encode(buf, input, default_decor.0)?;
        } else {
            write!(buf, ".")?;
            dotted_decor.prefix_encode(buf, input, DEFAULT_KEY_PATH_DECOR.0)?;
        }

        encode_key(key, buf, input)?;

        if last {
            leaf_decor.suffix_encode(buf, input, default_decor.1)?;
        } else {
            dotted_decor.suffix_encode(buf, input, DEFAULT_KEY_PATH_DECOR.1)?;
        }
    }
    Ok(())
}

pub(crate) fn encode_key_path_ref(
    this: &[&Key],
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    let leaf_decor = this.last().expect("always at least one key").leaf_decor();
    for (i, key) in this.iter().enumerate() {
        let dotted_decor = key.dotted_decor();

        let first = i == 0;
        let last = i + 1 == this.len();

        if first {
            leaf_decor.prefix_encode(buf, input, default_decor.0)?;
        } else {
            write!(buf, ".")?;
            dotted_decor.prefix_encode(buf, input, DEFAULT_KEY_PATH_DECOR.0)?;
        }

        encode_key(key, buf, input)?;

        if last {
            leaf_decor.suffix_encode(buf, input, default_decor.1)?;
        } else {
            dotted_decor.suffix_encode(buf, input, DEFAULT_KEY_PATH_DECOR.1)?;
        }
    }
    Ok(())
}

pub(crate) fn encode_formatted<T: ValueRepr>(
    this: &Formatted<T>,
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    let decor = this.decor();
    decor.prefix_encode(buf, input, default_decor.0)?;

    if let Some(input) = input {
        let repr = this
            .as_repr()
            .map(Cow::Borrowed)
            .unwrap_or_else(|| Cow::Owned(this.default_repr()));
        repr.encode(buf, input)?;
    } else {
        let repr = this.display_repr();
        write!(buf, "{}", repr)?;
    };

    decor.suffix_encode(buf, input, default_decor.1)?;
    Ok(())
}

pub(crate) fn encode_array(
    this: &Array,
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    let decor = this.decor();
    decor.prefix_encode(buf, input, default_decor.0)?;
    write!(buf, "[")?;

    for (i, elem) in this.iter().enumerate() {
        let inner_decor;
        if i == 0 {
            inner_decor = DEFAULT_LEADING_VALUE_DECOR;
        } else {
            inner_decor = DEFAULT_VALUE_DECOR;
            write!(buf, ",")?;
        }
        encode_value(elem, buf, input, inner_decor)?;
    }
    if this.trailing_comma() && !this.is_empty() {
        write!(buf, ",")?;
    }

    this.trailing().encode_with_default(buf, input, "")?;
    write!(buf, "]")?;
    decor.suffix_encode(buf, input, default_decor.1)?;

    Ok(())
}

pub(crate) fn encode_table(
    this: &InlineTable,
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    let decor = this.decor();
    decor.prefix_encode(buf, input, default_decor.0)?;
    write!(buf, "{{")?;
    this.preamble().encode_with_default(buf, input, "")?;

    let children = this.get_values();
    let len = children.len();
    for (i, (key_path, value)) in children.into_iter().enumerate() {
        if i != 0 {
            write!(buf, ",")?;
        }
        let inner_decor = if i == len - 1 {
            DEFAULT_TRAILING_VALUE_DECOR
        } else {
            DEFAULT_VALUE_DECOR
        };
        encode_key_path_ref(&key_path, buf, input, DEFAULT_INLINE_KEY_DECOR)?;
        write!(buf, "=")?;
        encode_value(value, buf, input, inner_decor)?;
    }

    write!(buf, "}}")?;
    decor.suffix_encode(buf, input, default_decor.1)?;

    Ok(())
}

pub(crate) fn encode_value(
    this: &Value,
    buf: &mut dyn Write,
    input: Option<&str>,
    default_decor: (&str, &str),
) -> Result {
    match this {
        Value::String(repr) => encode_formatted(repr, buf, input, default_decor),
        Value::Integer(repr) => encode_formatted(repr, buf, input, default_decor),
        Value::Float(repr) => encode_formatted(repr, buf, input, default_decor),
        Value::Boolean(repr) => encode_formatted(repr, buf, input, default_decor),
        Value::Datetime(repr) => encode_formatted(repr, buf, input, default_decor),
        Value::Array(array) => encode_array(array, buf, input, default_decor),
        Value::InlineTable(table) => encode_table(table, buf, input, default_decor),
    }
}

impl Display for DocumentMut {
    fn fmt(&self, f: &mut Formatter<'_>) -> Result {
        let decor = self.decor();
        decor.prefix_encode(f, None, DEFAULT_ROOT_DECOR.0)?;

        let mut path = Vec::new();
        let mut last_position = 0;
        let mut tables = Vec::new();
        visit_nested_tables(self.as_table(), &mut path, false, &mut |t, p, is_array| {
            if let Some(pos) = t.position() {
                last_position = pos;
            }
            tables.push((last_position, t, p.clone(), is_array));
            Ok(())
        })
        .unwrap();

        tables.sort_by_key(|&(id, _, _, _)| id);
        let mut first_table = true;
        for (_, table, path, is_array) in tables {
            visit_table(f, None, table, &path, is_array, &mut first_table)?;
        }
        decor.suffix_encode(f, None, DEFAULT_ROOT_DECOR.1)?;
        self.trailing().encode_with_default(f, None, "")
    }
}

fn visit_nested_tables<'t, F>(
    table: &'t Table,
    path: &mut Vec<Key>,
    is_array_of_tables: bool,
    callback: &mut F,
) -> Result
where
    F: FnMut(&'t Table, &Vec<Key>, bool) -> Result,
{
    if !table.is_dotted() {
        callback(table, path, is_array_of_tables)?;
    }

    for (key, value) in table.items.iter() {
        match value {
            Item::Table(ref t) => {
                let key = key.clone();
                path.push(key);
                visit_nested_tables(t, path, false, callback)?;
                path.pop();
            }
            Item::ArrayOfTables(ref a) => {
                for t in a.iter() {
                    let key = key.clone();
                    path.push(key);
                    visit_nested_tables(t, path, true, callback)?;
                    path.pop();
                }
            }
            _ => {}
        }
    }
    Ok(())
}

fn visit_table(
    buf: &mut dyn Write,
    input: Option<&str>,
    table: &Table,
    path: &[Key],
    is_array_of_tables: bool,
    first_table: &mut bool,
) -> Result {
    let children = table.get_values();
    // We are intentionally hiding implicit tables without any tables nested under them (ie
    // `table.is_empty()` which is in contrast to `table.get_values().is_empty()`).  We are
    // trusting the user that an empty implicit table is not semantically meaningful
    //
    // This allows a user to delete all tables under this implicit table and the implicit table
    // will disappear.
    //
    // However, this means that users need to take care in deciding what tables get marked as
    // implicit.
    let is_visible_std_table = !(table.implicit && children.is_empty());

    if path.is_empty() {
        // don't print header for the root node
        if !children.is_empty() {
            *first_table = false;
        }
    } else if is_array_of_tables {
        let default_decor = if *first_table {
            *first_table = false;
            ("", DEFAULT_TABLE_DECOR.1)
        } else {
            DEFAULT_TABLE_DECOR
        };
        table.decor.prefix_encode(buf, input, default_decor.0)?;
        write!(buf, "[[")?;
        encode_key_path(path, buf, input, DEFAULT_KEY_PATH_DECOR)?;
        write!(buf, "]]")?;
        table.decor.suffix_encode(buf, input, default_decor.1)?;
        writeln!(buf)?;
    } else if is_visible_std_table {
        let default_decor = if *first_table {
            *first_table = false;
            ("", DEFAULT_TABLE_DECOR.1)
        } else {
            DEFAULT_TABLE_DECOR
        };
        table.decor.prefix_encode(buf, input, default_decor.0)?;
        write!(buf, "[")?;
        encode_key_path(path, buf, input, DEFAULT_KEY_PATH_DECOR)?;
        write!(buf, "]")?;
        table.decor.suffix_encode(buf, input, default_decor.1)?;
        writeln!(buf)?;
    }
    // print table body
    for (key_path, value) in children {
        encode_key_path_ref(&key_path, buf, input, DEFAULT_KEY_DECOR)?;
        write!(buf, "=")?;
        encode_value(value, buf, input, DEFAULT_VALUE_DECOR)?;
        writeln!(buf)?;
    }
    Ok(())
}

impl ValueRepr for String {
    fn to_repr(&self) -> Repr {
        to_string_repr(self, None, None)
    }
}

pub(crate) fn to_string_repr(
    value: &str,
    style: Option<StringStyle>,
    literal: Option<bool>,
) -> Repr {
    let (style, literal) = infer_style(value, style, literal);

    let mut output = String::with_capacity(value.len() * 2);
    if literal {
        output.push_str(style.literal_start());
        output.push_str(value);
        output.push_str(style.literal_end());
    } else {
        output.push_str(style.standard_start());
        for ch in value.chars() {
            match ch {
                '\u{8}' => output.push_str("\\b"),
                '\u{9}' => output.push_str("\\t"),
                '\u{a}' => match style {
                    StringStyle::NewlineTriple => output.push('\n'),
                    StringStyle::OnelineSingle => output.push_str("\\n"),
                    StringStyle::OnelineTriple => unreachable!(),
                },
                '\u{c}' => output.push_str("\\f"),
                '\u{d}' => output.push_str("\\r"),
                '\u{22}' => output.push_str("\\\""),
                '\u{5c}' => output.push_str("\\\\"),
                c if c <= '\u{1f}' || c == '\u{7f}' => {
                    write!(output, "\\u{:04X}", ch as u32).unwrap();
                }
                ch => output.push(ch),
            }
        }
        output.push_str(style.standard_end());
    }

    Repr::new_unchecked(output)
}

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub(crate) enum StringStyle {
    NewlineTriple,
    OnelineTriple,
    OnelineSingle,
}

impl StringStyle {
    fn literal_start(self) -> &'static str {
        match self {
            Self::NewlineTriple => "'''\n",
            Self::OnelineTriple => "'''",
            Self::OnelineSingle => "'",
        }
    }
    fn literal_end(self) -> &'static str {
        match self {
            Self::NewlineTriple => "'''",
            Self::OnelineTriple => "'''",
            Self::OnelineSingle => "'",
        }
    }

    fn standard_start(self) -> &'static str {
        match self {
            Self::NewlineTriple => "\"\"\"\n",
            // note: OnelineTriple can happen if do_pretty wants to do
            // '''it's one line'''
            // but literal == false
            Self::OnelineTriple | Self::OnelineSingle => "\"",
        }
    }

    fn standard_end(self) -> &'static str {
        match self {
            Self::NewlineTriple => "\"\"\"",
            // note: OnelineTriple can happen if do_pretty wants to do
            // '''it's one line'''
            // but literal == false
            Self::OnelineTriple | Self::OnelineSingle => "\"",
        }
    }
}

fn infer_style(
    value: &str,
    style: Option<StringStyle>,
    literal: Option<bool>,
) -> (StringStyle, bool) {
    match (style, literal) {
        (Some(style), Some(literal)) => (style, literal),
        (None, Some(literal)) => (infer_all_style(value).0, literal),
        (Some(style), None) => {
            let literal = infer_literal(value);
            (style, literal)
        }
        (None, None) => infer_all_style(value),
    }
}

fn infer_literal(value: &str) -> bool {
    #[cfg(feature = "parse")]
    {
        use winnow::stream::ContainsToken as _;
        (value.contains('"') | value.contains('\\'))
            && value
                .chars()
                .all(|c| crate::parser::strings::LITERAL_CHAR.contains_token(c))
    }
    #[cfg(not(feature = "parse"))]
    {
        false
    }
}

fn infer_all_style(value: &str) -> (StringStyle, bool) {
    // We need to determine:
    // - if we are a "multi-line" pretty (if there are \n)
    // - if ['''] appears if multi or ['] if single
    // - if there are any invalid control characters
    //
    // Doing it any other way would require multiple passes
    // to determine if a pretty string works or not.
    let mut ty = StringStyle::OnelineSingle;
    // found consecutive single quotes
    let mut max_found_singles = 0;
    let mut found_singles = 0;
    let mut prefer_literal = false;
    let mut can_be_pretty = true;

    for ch in value.chars() {
        if can_be_pretty {
            if ch == '\'' {
                found_singles += 1;
                if found_singles >= 3 {
                    can_be_pretty = false;
                }
            } else {
                if found_singles > max_found_singles {
                    max_found_singles = found_singles;
                }
                found_singles = 0;
            }
            match ch {
                '\t' => {}
                '"' => {
                    prefer_literal = true;
                }
                '\\' => {
                    prefer_literal = true;
                }
                '\n' => ty = StringStyle::NewlineTriple,
                // Escape codes are needed if any ascii control
                // characters are present, including \b \f \r.
                c if c <= '\u{1f}' || c == '\u{7f}' => can_be_pretty = false,
                _ => {}
            }
        } else {
            // the string cannot be represented as pretty,
            // still check if it should be multiline
            if ch == '\n' {
                ty = StringStyle::NewlineTriple;
            }
        }
    }
    if found_singles > 0 && value.ends_with('\'') {
        // We cannot escape the ending quote so we must use """
        can_be_pretty = false;
    }
    if !prefer_literal {
        can_be_pretty = false;
    }
    if !can_be_pretty {
        debug_assert!(ty != StringStyle::OnelineTriple);
        return (ty, false);
    }
    if found_singles > max_found_singles {
        max_found_singles = found_singles;
    }
    debug_assert!(max_found_singles < 3);
    if ty == StringStyle::OnelineSingle && max_found_singles >= 1 {
        // no newlines, but must use ''' because it has ' in it
        ty = StringStyle::OnelineTriple;
    }
    (ty, true)
}

impl ValueRepr for i64 {
    fn to_repr(&self) -> Repr {
        Repr::new_unchecked(self.to_string())
    }
}

impl ValueRepr for f64 {
    fn to_repr(&self) -> Repr {
        to_f64_repr(*self)
    }
}

fn to_f64_repr(f: f64) -> Repr {
    let repr = match (f.is_sign_negative(), f.is_nan(), f == 0.0) {
        (true, true, _) => "-nan".to_owned(),
        (false, true, _) => "nan".to_owned(),
        (true, false, true) => "-0.0".to_owned(),
        (false, false, true) => "0.0".to_owned(),
        (_, false, false) => {
            if f % 1.0 == 0.0 {
                format!("{}.0", f)
            } else {
                format!("{}", f)
            }
        }
    };
    Repr::new_unchecked(repr)
}

impl ValueRepr for bool {
    fn to_repr(&self) -> Repr {
        Repr::new_unchecked(self.to_string())
    }
}

impl ValueRepr for Datetime {
    fn to_repr(&self) -> Repr {
        Repr::new_unchecked(self.to_string())
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use proptest::prelude::*;

    proptest! {
        #[test]
        #[cfg(feature = "parse")]
        fn parseable_string(string in "\\PC*") {
            let string = Value::from(string);
            let encoded = string.to_string();
            let _: Value = encoded.parse().unwrap_or_else(|err| {
                panic!("error: {err}

string:
```
{string}
```
")
            });
        }
    }

    proptest! {
        #[test]
        #[cfg(feature = "parse")]
        fn parseable_key(string in "\\PC*") {
            let string = Key::new(string);
            let encoded = string.to_string();
            let _: Key = encoded.parse().unwrap_or_else(|err| {
                panic!("error: {err}

string:
```
{string}
```
")
            });
        }
    }
}
