use alloc::{
    collections::BTreeMap,
    string::{String, ToString},
};
use borsh::{from_slice, to_vec, BorshDeserialize, BorshSerialize};

// the `BorshSchema` derive expands to code that uses the `format!` macro at the
// struct definition site, so it has to be in scope here too.
#[cfg(feature = "unstable__schema")]
use alloc::format;

#[derive(Debug, PartialEq, Eq)]
struct ThirdParty<K, V>(pub BTreeMap<K, V>);

mod third_party_impl {
    use super::ThirdParty;

    pub(super) fn serialize_third_party<
        K: borsh::ser::BorshSerialize,
        V: borsh::ser::BorshSerialize,
        W: borsh::io::Write,
    >(
        obj: &ThirdParty<K, V>,
        writer: &mut W,
    ) -> ::core::result::Result<(), borsh::io::Error> {
        borsh::BorshSerialize::serialize(&obj.0, writer)?;
        Ok(())
    }

    pub(super) fn deserialize_third_party<
        R: borsh::io::Read,
        K: borsh::de::BorshDeserialize + Ord,
        V: borsh::de::BorshDeserialize,
    >(
        reader: &mut R,
    ) -> ::core::result::Result<ThirdParty<K, V>, borsh::io::Error> {
        Ok(ThirdParty(borsh::BorshDeserialize::deserialize_reader(
            reader,
        )?))
    }

    #[cfg(feature = "unstable__schema")]
    use alloc::{collections::BTreeMap, format, vec};

    #[cfg(feature = "unstable__schema")]
    pub(super) fn declaration<K: borsh::BorshSchema, V: borsh::BorshSchema>(
    ) -> borsh::schema::Declaration {
        let params = vec![<K>::declaration(), <V>::declaration()];
        format!(r#"{}<{}>"#, "ThirdParty", params.join(", "))
    }

    #[cfg(feature = "unstable__schema")]
    pub(super) fn add_definitions_recursively<K: borsh::BorshSchema, V: borsh::BorshSchema>(
        definitions: &mut BTreeMap<borsh::schema::Declaration, borsh::schema::Definition>,
    ) {
        let fields = borsh::schema::Fields::UnnamedFields(vec![
            <BTreeMap<K, V> as borsh::BorshSchema>::declaration(),
        ]);
        let definition = borsh::schema::Definition::Struct { fields };
        let no_recursion_flag = definitions.get(&declaration::<K, V>()).is_none();
        borsh::schema::add_definition(declaration::<K, V>(), definition, definitions);
        if no_recursion_flag {
            <BTreeMap<K, V> as borsh::BorshSchema>::add_definitions_recursively(definitions);
        }
    }
}

// This mirrors the `near/intents` use case: the common `serialize_with` /
// `deserialize_with` / `bound` part is written once, and the schema-only part
// lives in a separate, `cfg`-gated `#[borsh(...)]` attribute. borsh merges the
// disjoint top-level keys of all `#[borsh(...)]` attributes into one.
#[cfg_attr(feature = "unstable__schema", derive(borsh::BorshSchema))]
#[derive(BorshSerialize, BorshDeserialize, PartialEq, Eq, Debug)]
struct A<K, V> {
    #[borsh(serialize_with = "third_party_impl::serialize_third_party")]
    #[borsh(deserialize_with = "third_party_impl::deserialize_third_party")]
    #[borsh(bound(
        deserialize = "K: borsh::de::BorshDeserialize + Ord, V: borsh::de::BorshDeserialize",
    ))]
    #[cfg_attr(
        feature = "unstable__schema",
        borsh(schema(with_funcs(
            declaration = "third_party_impl::declaration::<K, V>",
            definitions = "third_party_impl::add_definitions_recursively::<K, V>"
        )))
    )]
    x: ThirdParty<K, V>,
    y: u64,
}

#[test]
fn test_overridden_struct_multiple_attrs() {
    let mut m = BTreeMap::<u64, String>::new();
    m.insert(0, "0th element".to_string());
    m.insert(1, "1st element".to_string());
    let th_p = ThirdParty(m);
    let a = A { x: th_p, y: 42 };

    let data = to_vec(&a).unwrap();

    let actual_a = from_slice::<A<u64, String>>(&data).unwrap();
    assert_eq!(a, actual_a);
}

#[cfg(feature = "unstable__schema")]
#[test]
fn test_overridden_struct_multiple_attrs_schema() {
    use borsh::BorshSchema;

    assert_eq!(
        "A<u64, String>".to_string(),
        <A<u64, String>>::declaration()
    );
    let mut defs = Default::default();
    <A<u64, String>>::add_definitions_recursively(&mut defs);
    // the schema-only `with_funcs` attribute from the separate `#[borsh(...)]`
    // block takes effect: `x` resolves to the custom `ThirdParty` declaration.
    assert!(defs.contains_key("ThirdParty<u64, String>"));
}

// Item-level merge through the real derive macro: `use_discriminant` lives in
// one `#[borsh(...)]` attribute and `init` in a separate one. Both have to take
// effect for the assertions below to hold.
#[derive(BorshSerialize, BorshDeserialize, PartialEq, Eq, Debug)]
#[borsh(use_discriminant = true)]
#[borsh(init = initialization_method)]
enum WithSplitItemAttrs {
    A,
    B = 10,
    C,
}

impl WithSplitItemAttrs {
    fn initialization_method(&mut self) {
        *self = WithSplitItemAttrs::C;
    }
}

#[test]
fn test_item_level_multiple_attrs() {
    let value = WithSplitItemAttrs::B;
    let data = to_vec(&value).unwrap();

    // `use_discriminant = true` (first attribute) honors the explicit
    // discriminant: `B = 10` serializes its tag as the byte `10`, not the
    // positional index `1`.
    assert_eq!(data, [10]);

    // `init = initialization_method` (second attribute) runs after decoding, so
    // the deserialized value is rewritten to `C`.
    let decoded = from_slice::<WithSplitItemAttrs>(&data).unwrap();
    assert_eq!(decoded, WithSplitItemAttrs::C);
}
