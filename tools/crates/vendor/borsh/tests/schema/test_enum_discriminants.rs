use crate::common_macro::schema_imports::*;

#[allow(unused)]
#[derive(BorshSchema)]
#[borsh(use_discriminant = true)]
#[repr(i16)]
enum XY {
    A,
    B = 20,
    C,
    D(u32, u32),
    E = 10,
    F(u64),
}

#[test]
fn test_schema_discriminant_no_unit_type() {
    assert_eq!("XY".to_string(), XY::declaration());
    let mut defs = Default::default();
    XY::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "XY" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                     (0, "A".to_string(), "XY__A".to_string()),
                     (20, "B".to_string(), "XY__B".to_string()),
                     (21, "C".to_string(), "XY__C".to_string()),
                     (22, "D".to_string(), "XY__D".to_string()),
                     (10, "E".to_string(), "XY__E".to_string()),
                     (11, "F".to_string(), "XY__F".to_string())
                ]
            },
            "XY__A" => Definition::Struct{ fields: Fields::Empty },
            "XY__B" => Definition::Struct{ fields: Fields::Empty },
            "XY__C" => Definition::Struct{ fields: Fields::Empty },
            "XY__D" => Definition::Struct{ fields: Fields::UnnamedFields(
                vec!["u32".to_string(), "u32".to_string()]
            )},
            "XY__E" => Definition::Struct{ fields: Fields::Empty },
            "XY__F" => Definition::Struct{ fields: Fields::UnnamedFields(
                vec!["u64".to_string()]

            )},
            "u32" => Definition::Primitive(4),
            "u64" => Definition::Primitive(8)
        },
        defs
    );
}

#[allow(unused)]
#[derive(BorshSchema)]
#[borsh(use_discriminant = false)]
#[repr(i16)]
enum XYNoDiscriminant {
    A,
    B = 20,
    C,
    D(u32, u32),
    E = 10,
    F(u64),
}

#[test]
fn test_schema_discriminant_no_unit_type_no_use_discriminant() {
    assert_eq!(
        "XYNoDiscriminant".to_string(),
        XYNoDiscriminant::declaration()
    );
    let mut defs = Default::default();
    XYNoDiscriminant::add_definitions_recursively(&mut defs);
    assert_eq!(
        schema_map! {
            "XYNoDiscriminant" => Definition::Enum {
                tag_width: 1,
                variants: vec![
                     (0, "A".to_string(), "XYNoDiscriminant__A".to_string()),
                     (1, "B".to_string(), "XYNoDiscriminant__B".to_string()),
                     (2, "C".to_string(), "XYNoDiscriminant__C".to_string()),
                     (3, "D".to_string(), "XYNoDiscriminant__D".to_string()),
                     (4, "E".to_string(), "XYNoDiscriminant__E".to_string()),
                     (5, "F".to_string(), "XYNoDiscriminant__F".to_string())
                ]
            },
            "XYNoDiscriminant__A" => Definition::Struct{ fields: Fields::Empty },
            "XYNoDiscriminant__B" => Definition::Struct{ fields: Fields::Empty },
            "XYNoDiscriminant__C" => Definition::Struct{ fields: Fields::Empty },
            "XYNoDiscriminant__D" => Definition::Struct{ fields: Fields::UnnamedFields(
                vec!["u32".to_string(), "u32".to_string()]
            )},
            "XYNoDiscriminant__E" => Definition::Struct{ fields: Fields::Empty },
            "XYNoDiscriminant__F" => Definition::Struct{ fields: Fields::UnnamedFields(
                vec!["u64".to_string()]

            )},
            "u32" => Definition::Primitive(4),
            "u64" => Definition::Primitive(8)
        },
        defs
    );
}
