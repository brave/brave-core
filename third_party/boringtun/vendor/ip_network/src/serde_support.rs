use std::fmt;
use std::str;
use serde::de::{Deserialize, Deserializer, EnumAccess, Error, Unexpected, VariantAccess, Visitor};
use serde::ser::{Serialize, Serializer};
use crate::{IpNetwork, Ipv4Network, Ipv6Network};

impl Serialize for IpNetwork {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        if serializer.is_human_readable() {
            serializer.serialize_str(&self.to_string())
        } else {
            match self {
                IpNetwork::V4(a) => serializer.serialize_newtype_variant("IpNetwork", 0, "V4", a),
                IpNetwork::V6(a) => serializer.serialize_newtype_variant("IpNetwork", 1, "V6", a),
            }
        }
    }
}

impl<'de> Deserialize<'de> for IpNetwork {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        if deserializer.is_human_readable() {
            struct IpNetworkVisitor;

            impl<'de> Visitor<'de> for IpNetworkVisitor {
                type Value = IpNetwork;

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    formatter.write_str("IP network")
                }

                fn visit_str<E>(self, s: &str) -> Result<Self::Value, E>
                where
                    E: Error,
                {
                    s.parse().map_err(Error::custom)
                }
            }

            deserializer.deserialize_str(IpNetworkVisitor)
        } else {
            enum IpNetworkKind {
                V4,
                V6,
            }

            static VARIANTS: &[&str] = &["V4", "V6"];

            impl<'de> Deserialize<'de> for IpNetworkKind {
                fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
                where
                    D: Deserializer<'de>,
                {
                    struct KindVisitor;
                    impl<'de> Visitor<'de> for KindVisitor {
                        type Value = IpNetworkKind;
                        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                            formatter.write_str("`V4` or `V6`")
                        }
                        fn visit_u32<E: Error>(self, value: u32) -> Result<Self::Value, E> {
                            match value {
                                0 => Ok(IpNetworkKind::V4),
                                1 => Ok(IpNetworkKind::V6),
                                _ => Err(Error::invalid_value(
                                    Unexpected::Unsigned(value as u64),
                                    &self,
                                )),
                            }
                        }
                        fn visit_str<E: Error>(self, value: &str) -> Result<Self::Value, E> {
                            match value {
                                "V4" => Ok(IpNetworkKind::V4),
                                "V6" => Ok(IpNetworkKind::V6),
                                _ => Err(Error::unknown_variant(value, VARIANTS)),
                            }
                        }
                        fn visit_bytes<E: Error>(self, value: &[u8]) -> Result<Self::Value, E> {
                            match value {
                                b"V4" => Ok(IpNetworkKind::V4),
                                b"V6" => Ok(IpNetworkKind::V6),
                                _ => match str::from_utf8(value) {
                                    Ok(value) => Err(Error::unknown_variant(value, VARIANTS)),
                                    Err(_) => {
                                        Err(Error::invalid_value(Unexpected::Bytes(value), &self))
                                    }
                                },
                            }
                        }
                    }
                    deserializer.deserialize_identifier(KindVisitor)
                }
            }

            struct EnumVisitor;

            impl<'de> Visitor<'de> for EnumVisitor {
                type Value = IpNetwork;

                fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                    formatter.write_str("IP network")
                }

                fn visit_enum<A>(self, data: A) -> Result<Self::Value, A::Error>
                where
                    A: EnumAccess<'de>,
                {
                    match data.variant()? {
                        (IpNetworkKind::V4, v) => v.newtype_variant().map(IpNetwork::V4),
                        (IpNetworkKind::V6, v) => v.newtype_variant().map(IpNetwork::V6),
                    }
                }
            }

            deserializer.deserialize_enum("IpNetwork", VARIANTS, EnumVisitor)
        }
    }
}

macro_rules! ser_de_impl {
    ($expecting:tt $ty:ty) => {
        impl Serialize for $ty {
            fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: Serializer,
            {
                if serializer.is_human_readable() {
                    serializer.serialize_str(&self.to_string())
                } else {
                    (self.network_address(), self.netmask()).serialize(serializer)
                }
            }
        }

        impl<'de> Deserialize<'de> for $ty {
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
            where
                D: Deserializer<'de>,
            {
                if deserializer.is_human_readable() {
                    struct IpNetworkVisitor;

                    impl<'de> Visitor<'de> for IpNetworkVisitor {
                        type Value = $ty;

                        fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                            formatter.write_str($expecting)
                        }

                        fn visit_str<E>(self, s: &str) -> Result<Self::Value, E>
                        where
                            E: Error,
                        {
                            s.parse().map_err(Error::custom)
                        }
                    }

                    deserializer.deserialize_str(IpNetworkVisitor)
                } else {
                    let (network_address, netmask) = <(_, u8)>::deserialize(deserializer)?;
                    Self::new(network_address, netmask).map_err(Error::custom)
                }
            }
        }
    };
}

ser_de_impl!("IPv4 network" Ipv4Network);
ser_de_impl!("IPv6 network" Ipv6Network);

#[cfg(test)]
mod tests {
    use crate::{IpNetwork, Ipv4Network, Ipv6Network};
    use serde_test::{assert_tokens, Configure, Token};
    use std::net::{Ipv4Addr, Ipv6Addr};

    #[test]
    fn ip_network_serialize_readable() {
        let ip_network = IpNetwork::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();

        assert_tokens(&ip_network.readable(), &[Token::BorrowedStr("1.2.3.4/32")]);
    }

    #[test]
    fn ip_network_serialize_compact() {
        let ip_network = IpNetwork::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();

        assert_tokens(
            &ip_network.compact(),
            &[
                Token::NewtypeVariant {
                    name: "IpNetwork",
                    variant: "V4",
                },
                Token::Tuple { len: 2 },
                Token::Tuple { len: 4 },
                Token::U8(1),
                Token::U8(2),
                Token::U8(3),
                Token::U8(4),
                Token::TupleEnd,
                Token::U8(32),
                Token::TupleEnd,
            ],
        );
    }

    #[test]
    fn ipv4_network_serialize_readable() {
        let ip_network = Ipv4Network::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();

        assert_tokens(&ip_network.readable(), &[Token::BorrowedStr("1.2.3.4/32")]);
    }

    #[test]
    fn ipv4_network_serialize_compact() {
        let ip_network = Ipv4Network::new(Ipv4Addr::new(1, 2, 3, 4), 32).unwrap();

        assert_tokens(
            &ip_network.compact(),
            &[
                Token::Tuple { len: 2 },
                Token::Tuple { len: 4 },
                Token::U8(1),
                Token::U8(2),
                Token::U8(3),
                Token::U8(4),
                Token::TupleEnd,
                Token::U8(32),
                Token::TupleEnd,
            ],
        );
    }

    #[test]
    fn ipv6_network_serialize_readable() {
        let ip_network = Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 0, 0, 0, 0), 64).unwrap();

        assert_tokens(
            &ip_network.readable(),
            &[Token::BorrowedStr("1:2:3:4::/64")],
        );
    }

    #[test]
    fn ipv6_network_serialize_compact() {
        let ip_network = Ipv6Network::new(Ipv6Addr::new(1, 2, 3, 4, 0, 0, 0, 0), 64).unwrap();

        assert_tokens(
            &ip_network.compact(),
            &[
                Token::Tuple { len: 2 },
                Token::Tuple { len: 16 },
                Token::U8(0),
                Token::U8(1),
                Token::U8(0),
                Token::U8(2),
                Token::U8(0),
                Token::U8(3),
                Token::U8(0),
                Token::U8(4),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::U8(0),
                Token::TupleEnd,
                Token::U8(64),
                Token::TupleEnd,
            ],
        );
    }
}
