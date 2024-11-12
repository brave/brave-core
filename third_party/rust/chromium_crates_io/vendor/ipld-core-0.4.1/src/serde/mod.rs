//! Serde (de)serialization for [`crate::ipld::Ipld`].
//!
//! This implementation enables Serde to serialize to/deserialize from [`crate::ipld::Ipld`]
//! values. The `Ipld` enum is similar to the `Value` enum in `serde_json` or `serde_cbor`.
mod de;
mod extract_links;
mod ser;

use alloc::string::{String, ToString};
use core::fmt;

pub use de::from_ipld;
pub use extract_links::ExtractLinks;
pub use ser::{to_ipld, Serializer};

/// Error during Serde operations.
#[derive(Clone, Debug)]
pub struct SerdeError(String);

impl fmt::Display for SerdeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "serde error: {}", self.0)
    }
}

impl serde::de::Error for SerdeError {
    fn custom<T: fmt::Display>(message: T) -> Self {
        Self(message.to_string())
    }
}

impl serde::ser::Error for SerdeError {
    fn custom<T: fmt::Display>(message: T) -> Self {
        Self(message.to_string())
    }
}

impl serde::ser::StdError for SerdeError {}

#[cfg(test)]
mod tests {
    use alloc::{collections::BTreeMap, string::String, vec, vec::Vec};
    use core::fmt;

    use cid::serde::CID_SERDE_PRIVATE_IDENTIFIER;
    use cid::Cid;
    use serde::{de::DeserializeOwned, Serialize};
    use serde_derive::Deserialize;
    use serde_test::{assert_tokens, Token};

    use crate::ipld::Ipld;
    use crate::serde::{from_ipld, to_ipld};

    /// Utility for testing (de)serialization of [`Ipld`].
    ///
    /// Checks if `data` and `ipld` match if they are encoded into each other.
    fn assert_roundtrip<T>(data: &T, ipld: &Ipld)
    where
        T: Serialize + DeserializeOwned + PartialEq + fmt::Debug,
    {
        let encoded: Ipld = to_ipld(data).unwrap();
        assert_eq!(&encoded, ipld);
        let decoded: T = from_ipld(ipld.clone()).unwrap();
        assert_eq!(&decoded, data);
    }

    #[derive(Debug, Deserialize, PartialEq, Serialize)]
    struct Person {
        name: String,
        age: u8,
        hobbies: Vec<String>,
        is_cool: bool,
        link: Cid,
    }

    impl Default for Person {
        fn default() -> Self {
            Self {
                name: "Hello World!".into(),
                age: 52,
                hobbies: vec!["geography".into(), "programming".into()],
                is_cool: true,
                link: Cid::try_from("bafyreibvjvcv745gig4mvqs4hctx4zfkono4rjejm2ta6gtyzkqxfjeily")
                    .unwrap(),
            }
        }
    }

    #[test]
    fn test_tokens() {
        let person = Person::default();

        assert_tokens(
            &person,
            &[
                Token::Struct {
                    name: "Person",
                    len: 5,
                },
                Token::Str("name"),
                Token::Str("Hello World!"),
                Token::Str("age"),
                Token::U8(52),
                Token::Str("hobbies"),
                Token::Seq { len: Some(2) },
                Token::Str("geography"),
                Token::Str("programming"),
                Token::SeqEnd,
                Token::Str("is_cool"),
                Token::Bool(true),
                Token::Str("link"),
                Token::NewtypeStruct {
                    name: CID_SERDE_PRIVATE_IDENTIFIER,
                },
                Token::Bytes(&[
                    0x01, 0x71, 0x12, 0x20, 0x35, 0x4d, 0x45, 0x5f, 0xf3, 0xa6, 0x41, 0xb8, 0xca,
                    0xc2, 0x5c, 0x38, 0xa7, 0x7e, 0x64, 0xaa, 0x73, 0x5d, 0xc8, 0xa4, 0x89, 0x66,
                    0xa6, 0xf, 0x1a, 0x78, 0xca, 0xa1, 0x72, 0xa4, 0x88, 0x5e,
                ]),
                Token::StructEnd,
            ],
        );
    }

    /// Test if converting to a struct from [`crate::ipld::Ipld`] and back works.
    #[test]
    fn test_ipld() {
        let person = Person::default();

        let expected_ipld = Ipld::Map({
            BTreeMap::from([
                ("name".into(), Ipld::String("Hello World!".into())),
                ("age".into(), Ipld::Integer(52)),
                (
                    "hobbies".into(),
                    Ipld::List(vec![
                        Ipld::String("geography".into()),
                        Ipld::String("programming".into()),
                    ]),
                ),
                ("is_cool".into(), Ipld::Bool(true)),
                ("link".into(), Ipld::Link(person.link)),
            ])
        });

        assert_roundtrip(&person, &expected_ipld);
    }

    /// Test that deserializing arbitrary bytes are not accidentally recognized as CID.
    #[test]
    fn test_bytes_not_cid() {
        let cid =
            Cid::try_from("bafyreibvjvcv745gig4mvqs4hctx4zfkono4rjejm2ta6gtyzkqxfjeily").unwrap();

        let bytes_not_cid = Ipld::Bytes(cid.to_bytes());
        let not_a_cid: Result<Cid, _> = from_ipld(bytes_not_cid);
        assert!(not_a_cid.is_err());

        // Make sure that a Ipld::Link deserializes correctly though.
        let link = Ipld::Link(cid);
        let a_cid: Cid = from_ipld(link).unwrap();
        assert_eq!(a_cid, cid);
    }
}
