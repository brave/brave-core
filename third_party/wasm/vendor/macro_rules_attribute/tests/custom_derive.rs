#[macro_use]
extern crate macro_rules_attribute;

derive_alias! {
    #[derive(Nothing!)] = #[derive()];
}

#[derive(Nothing!)]
#[custom(hello)]
struct Foo {
    #[derive_args("test")]
    x: i32,
}

#[macro_rules_derive(Nothing)]
#[custom(hello)]
struct Bar {
    #[derive_args("test")]
    x: i32,
}
