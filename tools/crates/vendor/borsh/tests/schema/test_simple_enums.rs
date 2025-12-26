use crate::common_macro::schema_imports::*;

use borsh::{try_from_slice_with_schema, try_to_vec_with_schema};

#[test]
pub fn simple_enum() {
    #[allow(dead_code)]
    #[derive(borsh::BorshSchema)]
    enum A {
        Bacon,
        Eggs,
    }
    // https://github.com/near/borsh-rs/issues/112
    #[allow(unused)]
    impl A {
        pub fn declaration() -> usize {
            42
        }
    }
    assert_eq!("A".to_string(), <A as borsh::BorshSchema>::declaration());
    let mut defs = Default::default();
    A::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
        "A__Bacon" => Definition::Struct{ fields: Fields::Empty },
        "A__Eggs" => Definition::Struct{ fields: Fields::Empty },
            "A" => Definition::Enum {
                tag_width: 1,
                variants: vec![(0, "Bacon".to_string(), "A__Bacon".to_string()), (1, "Eggs".to_string(), "A__Eggs".to_string())]
            }
        },
        defs
    );
}

#[test]
pub fn shadow_enum() {
    #[allow(dead_code)]
    #[derive(borsh::BorshSchema)]
    enum State {
        V1(StateV1),
    }
    #[derive(borsh::BorshSchema)]
    struct StateV1;

    assert_eq!(
        "State".to_string(),
        <State as borsh::BorshSchema>::declaration()
    );
    let mut defs = Default::default();
    State::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "State" => Definition::Enum {
                tag_width: 1,
                variants: vec![(0, "V1".to_string(), "State__V1".to_string())]
            },
            "State__V1" => Definition::Struct {
                fields: Fields::UnnamedFields(["StateV1".to_string()].into())
            },
            "StateV1" => Definition::Struct{ fields: Fields::Empty }
        },
        defs
    );
}

#[test]
pub fn single_field_enum() {
    #[allow(dead_code)]
    #[derive(borsh::BorshSchema)]
    enum A {
        Bacon,
    }
    assert_eq!("A".to_string(), A::declaration());
    let mut defs = Default::default();
    A::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "A__Bacon" => Definition::Struct {fields: Fields::Empty},
            "A" => Definition::Enum {
                tag_width: 1,
                variants: vec![(0, "Bacon".to_string(), "A__Bacon".to_string())]
            }
        },
        defs
    );
}

#[test]
pub fn complex_enum_with_schema() {
    #[derive(
        borsh::BorshSchema,
        Default,
        borsh::BorshSerialize,
        borsh::BorshDeserialize,
        PartialEq,
        Debug,
    )]
    struct Tomatoes;
    #[derive(
        borsh::BorshSchema,
        Default,
        borsh::BorshSerialize,
        borsh::BorshDeserialize,
        PartialEq,
        Debug,
    )]
    struct Cucumber;
    #[derive(
        borsh::BorshSchema,
        Default,
        borsh::BorshSerialize,
        borsh::BorshDeserialize,
        PartialEq,
        Debug,
    )]
    struct Oil;
    #[derive(
        borsh::BorshSchema,
        Default,
        borsh::BorshSerialize,
        borsh::BorshDeserialize,
        PartialEq,
        Debug,
    )]
    struct Wrapper;
    #[derive(
        borsh::BorshSchema,
        Default,
        borsh::BorshSerialize,
        borsh::BorshDeserialize,
        PartialEq,
        Debug,
    )]
    struct Filling;
    #[derive(
        borsh::BorshSchema, borsh::BorshSerialize, borsh::BorshDeserialize, PartialEq, Debug,
    )]
    enum A {
        Bacon,
        Eggs,
        Salad(Tomatoes, Cucumber, Oil),
        Sausage { wrapper: Wrapper, filling: Filling },
    }

    impl Default for A {
        fn default() -> Self {
            A::Sausage {
                wrapper: Default::default(),
                filling: Default::default(),
            }
        }
    }
    // First check schema.
    assert_eq!("A".to_string(), A::declaration());
    let mut defs = Default::default();
    A::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
        "Cucumber" => Definition::Struct {fields: Fields::Empty},
        "A__Salad" => Definition::Struct{ fields: Fields::UnnamedFields(vec!["Tomatoes".to_string(), "Cucumber".to_string(), "Oil".to_string()])},
        "A__Bacon" => Definition::Struct {fields: Fields::Empty},
        "Oil" => Definition::Struct {fields: Fields::Empty},
            "A" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                    (0, "Bacon".to_string(), "A__Bacon".to_string()),
                    (1, "Eggs".to_string(), "A__Eggs".to_string()),
                    (2, "Salad".to_string(), "A__Salad".to_string()),
                    (3, "Sausage".to_string(), "A__Sausage".to_string())
                ]
            },
        "Wrapper" => Definition::Struct {fields: Fields::Empty},
        "Tomatoes" => Definition::Struct {fields: Fields::Empty},
        "A__Sausage" => Definition::Struct { fields: Fields::NamedFields(vec![
        ("wrapper".to_string(), "Wrapper".to_string()),
        ("filling".to_string(), "Filling".to_string())
        ])},
        "A__Eggs" => Definition::Struct {fields: Fields::Empty},
        "Filling" => Definition::Struct {fields: Fields::Empty}
        },
        defs
    );
    // Then check that we serialize and deserialize with schema.
    let obj = A::default();
    let data = try_to_vec_with_schema(&obj).unwrap();
    #[cfg(feature = "std")]
    insta::assert_debug_snapshot!(data);
    let obj2: A = try_from_slice_with_schema(&data).unwrap();
    assert_eq!(obj, obj2);
}
