use snapbox::assert_data_eq;
use snapbox::prelude::*;
use snapbox::str;

use toml_edit::{DocumentMut, Item, Value};

#[test]
fn table_into_inline() {
    let toml = r#"
[table]
string = "value"
array = [1, 2, 3]
inline = { "1" = 1, "2" = 2 }

[table.child]
other = "world"
"#;
    let mut doc = toml.parse::<DocumentMut>().unwrap();

    doc.get_mut("table").unwrap().make_value();

    let actual = doc.to_string();
    // `table=` is because we didn't re-format the table key, only the value
    assert_data_eq!(actual, str![[r#"
table= { string = "value", array = [1, 2, 3], inline = { "1" = 1, "2" = 2 }, child = { other = "world" } }

"#]].raw());
}

#[test]
fn inline_table_to_table() {
    let toml = r#"table = { string = "value", array = [1, 2, 3], inline = { "1" = 1, "2" = 2 }, child = { other = "world" } }
"#;
    let mut doc = toml.parse::<DocumentMut>().unwrap();

    let t = doc.remove("table").unwrap();
    let t = match t {
        Item::Value(Value::InlineTable(t)) => t,
        _ => unreachable!("Unexpected {:?}", t),
    };
    let t = t.into_table();
    doc.insert("table", Item::Table(t));

    let actual = doc.to_string();
    assert_data_eq!(
        actual,
        str![[r#"
[table]
string = "value"
array = [1, 2, 3]
inline = { "1" = 1, "2" = 2 }
child = { other = "world" }

"#]]
        .raw()
    );
}

#[test]
fn array_of_tables_to_array() {
    let toml = r#"
[[table]]
string = "value"
array = [1, 2, 3]
inline = { "1" = 1, "2" = 2 }

[table.child]
other = "world"

[[table]]
string = "value"
array = [1, 2, 3]
inline = { "1" = 1, "2" = 2 }

[table.child]
other = "world"
"#;
    let mut doc = toml.parse::<DocumentMut>().unwrap();

    doc.get_mut("table").unwrap().make_value();

    let actual = doc.to_string();
    // `table=` is because we didn't re-format the table key, only the value
    assert_data_eq!(actual, str![[r#"
table= [{ string = "value", array = [1, 2, 3], inline = { "1" = 1, "2" = 2 }, child = { other = "world" } }, { string = "value", array = [1, 2, 3], inline = { "1" = 1, "2" = 2 }, child = { other = "world" } }]

"#]].raw());
}
