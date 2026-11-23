use crate::common_macro::schema_imports::*;

use core::marker::PhantomData;

#[test]
fn phantom_data_schema() {
    let phantom_declaration = PhantomData::<String>::declaration();
    assert_eq!("()", phantom_declaration);
    let phantom_declaration = PhantomData::<Vec<u8>>::declaration();
    assert_eq!("()", phantom_declaration);
}

#[test]
pub fn generic_struct_with_phantom_data_derived() {
    #[allow(unused)]
    #[derive(borsh::BorshSchema)]
    struct Parametrized<K, V> {
        field: K,
        another: PhantomData<V>,
    }

    assert_eq!(
        "Parametrized<String>".to_string(),
        <Parametrized<String, u32>>::declaration()
    );

    let mut defs = Default::default();
    <Parametrized<String, u32>>::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "Parametrized<String>" => Definition::Struct {
                fields: Fields::NamedFields(vec![
                    ("field".to_string(), "String".to_string()),
                    ("another".to_string(), "()".to_string())
                ])
            },
            "()" => Definition::Primitive(0),
            "String" => Definition::Sequence {
                length_width: Definition::DEFAULT_LENGTH_WIDTH,
                length_range: Definition::DEFAULT_LENGTH_RANGE,
                elements: "u8".to_string()
            },
            "u8" => Definition::Primitive(1)
        },
        defs
    );
}

#[test]
pub fn generic_enum_variant_with_phantom_data_derived() {
    #[allow(unused)]
    #[derive(borsh::BorshSchema)]
    enum Parametrized<T> {
        Item(PhantomData<T>),
    }

    struct Marker;

    assert_eq!(
        "Parametrized".to_string(),
        <Parametrized<Marker>>::declaration()
    );

    let mut defs = Default::default();
    <Parametrized<Marker>>::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "Parametrized" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                    (0, "Item".to_string(), "Parametrized__Item".to_string()),
                ],
            },
            "Parametrized__Item" => Definition::Struct {
                fields: Fields::UnnamedFields(vec!["()".to_string()])
            },
            "()" => Definition::Primitive(0)
        },
        defs
    );
}

#[test]
pub fn generic_enum_variant_with_mixed_phantom_data_predicate_derived() {
    #[allow(unused)]
    #[derive(borsh::BorshSchema)]
    enum Parametrized<T, U>
    where
        PhantomData<(T, U)>: Clone,
    {
        Item(PhantomData<T>),
        Other(PhantomData<U>),
    }

    struct Marker;

    assert_eq!(
        "Parametrized".to_string(),
        <Parametrized<Marker, Marker>>::declaration()
    );

    let mut defs = Default::default();
    <Parametrized<Marker, Marker>>::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "Parametrized" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                    (0, "Item".to_string(), "Parametrized__Item".to_string()),
                    (1, "Other".to_string(), "Parametrized__Other".to_string()),
                ],
            },
            "Parametrized__Item" => Definition::Struct {
                fields: Fields::UnnamedFields(vec!["()".to_string()])
            },
            "Parametrized__Other" => Definition::Struct {
                fields: Fields::UnnamedFields(vec!["()".to_string()])
            },
            "()" => Definition::Primitive(0)
        },
        defs
    );
}
