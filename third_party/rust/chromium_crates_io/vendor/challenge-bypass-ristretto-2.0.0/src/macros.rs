#[cfg(any(test, feature = "base64"))]
#[macro_export]
/// Implement the encode_base64 / decode_base64 functions for a struct which implements to_bytes / from_bytes
macro_rules! impl_base64 {
    ($t:ident) => {
        impl $t {
            #[cfg(all(feature = "alloc", not(feature = "std")))]
            /// Encode to a base64 string
            pub fn encode_base64(&self) -> ::alloc::string::String {
                ::base64::encode(&self.to_bytes()[..])
            }

            #[cfg(all(feature = "std"))]
            /// Encode to a base64 string
            pub fn encode_base64(&self) -> ::std::string::String {
                ::base64::encode(&self.to_bytes()[..])
            }

            /// Decode from a base64 string
            pub fn decode_base64(s: &str) -> Result<Self, TokenError> {
                let bytes =
                    ::base64::decode(s).or(Err(TokenError(InternalError::DecodingError)))?;
                $t::from_bytes(&bytes)
            }
        }
    };
}

#[cfg(all(feature = "serde", not(feature = "serde_base64")))]
#[macro_export]
/// Implement the Serialize / Deserialize traits for a struct which implements to_bytes / from_bytes
macro_rules! impl_serde {
    ($t:ident) => {
        impl ::serde::Serialize for $t {
            fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: ::serde::Serializer,
            {
                serializer.serialize_bytes(&self.to_bytes())
            }
        }

        impl<'d> ::serde::Deserialize<'d> for $t {
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
            where
                D: ::serde::Deserializer<'d>,
            {
                use core::fmt::Debug;

                struct TVisitor;

                impl<'d> ::serde::de::Visitor<'d> for TVisitor {
                    type Value = $t;

                    fn expecting(
                        &self,
                        formatter: &mut ::core::fmt::Formatter,
                    ) -> ::core::fmt::Result {
                        $t::bytes_length_error().fmt(formatter)
                    }

                    fn visit_bytes<E>(self, bytes: &[u8]) -> Result<$t, E>
                    where
                        E: ::serde::de::Error,
                    {
                        $t::from_bytes(bytes)
                            .or(Err(::serde::de::Error::invalid_length(bytes.len(), &self)))
                    }
                }
                deserializer.deserialize_bytes(TVisitor)
            }
        }
    };
}

#[cfg(feature = "serde_base64")]
#[macro_export]
/// Implement the Serialize / Deserialize traits for a struct which implements to_bytes / from_bytes
macro_rules! impl_serde {
    ($t:ident) => {
        impl ::serde::Serialize for $t {
            fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
            where
                S: ::serde::Serializer,
            {
                serializer.serialize_str(&self.encode_base64())
            }
        }

        impl<'d> ::serde::Deserialize<'d> for $t {
            fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
            where
                D: ::serde::Deserializer<'d>,
            {
                use core::fmt::Debug;

                struct TVisitor;

                impl<'d> ::serde::de::Visitor<'d> for TVisitor {
                    type Value = $t;

                    fn expecting(
                        &self,
                        formatter: &mut ::core::fmt::Formatter,
                    ) -> ::core::fmt::Result {
                        write!(formatter, "a base64 encoded string: ")?;
                        $t::bytes_length_error().fmt(formatter)
                    }

                    fn visit_str<E>(self, s: &str) -> Result<$t, E>
                    where
                        E: ::serde::de::Error,
                    {
                        $t::decode_base64(s).map_err(::serde::de::Error::custom)
                    }
                }
                deserializer.deserialize_str(TVisitor)
            }
        }
    };
}
