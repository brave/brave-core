use crate::common_macro::schema_imports::*;

// `Vec<Self>` must resolve to the enum, not to the synthesized per-variant inner struct, so
// that the schema matches the wire format (https://github.com/near/borsh-rs/issues/377).
#[allow(unused)]
#[derive(borsh::BorshSchema)]
enum ERecD {
    B { x: String, y: i32 },
    C(u8, Vec<Self>),
}

#[test]
pub fn recursive_enum_schema() {
    let mut defs = Default::default();
    ERecD::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
           "ERecD" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                    (0, "B".to_string(), "ERecD__B".to_string()),
                    (1, "C".to_string(), "ERecD__C".to_string()),
                ]
            },
            "ERecD__B" => Definition::Struct {
                fields: Fields::NamedFields (
                    vec![
                        ("x".to_string(), "String".to_string()),
                        ("y".to_string(), "i32".to_string()),
                    ]
                )
            },
            "ERecD__C" => Definition::Struct {
                fields: Fields::UnnamedFields( vec![
                    "u8".to_string(),
                    "Vec<ERecD>".to_string(),
                ])
            },
            "Vec<ERecD>" => Definition::Sequence {
                length_width: Definition::DEFAULT_LENGTH_WIDTH,
                length_range: Definition::DEFAULT_LENGTH_RANGE,
                elements: "ERecD".to_string(),
            },
            "i32" => Definition::Primitive(4),
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

// `Vec<Self>` in a generic enum must resolve to `List<T>` and keep `T` used, so the inner
// struct stays generic.
#[test]
pub fn recursive_generic_enum_self_schema() {
    #[allow(unused)]
    #[derive(borsh::BorshSchema)]
    enum List<T> {
        Nil,
        Cons(T, Vec<Self>),
    }

    let mut defs = Default::default();
    <List<u32>>::add_definitions_recursively(&mut defs);
    assert_eq!(
        Some(&Definition::Struct {
            fields: Fields::UnnamedFields(vec!["u32".to_string(), "Vec<List<u32>>".to_string()])
        }),
        defs.get("List__Cons<u32>")
    );
    assert_eq!(
        Some(&Definition::Sequence {
            length_width: Definition::DEFAULT_LENGTH_WIDTH,
            length_range: Definition::DEFAULT_LENGTH_RANGE,
            elements: "List<u32>".to_string(),
        }),
        defs.get("Vec<List<u32>>")
    );
}
