#[macro_use]
extern crate macro_rules_attribute;

use ::serde::Serialize;

derive_alias! {
    #[derive(Nothing!)] = #[derive()];
}

#[derive(Nothing!, Serialize)]
#[serde(rename_all = "snake_case")]
struct Foo {
    x: i32,
}
