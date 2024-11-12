//! Conversion to and from ipld.
use alloc::{
    borrow::ToOwned,
    boxed::Box,
    collections::BTreeMap,
    string::{String, ToString},
    vec::Vec,
};
use core::{any::TypeId, fmt};

use crate::{
    cid::Cid,
    ipld::{Ipld, IpldKind},
};

/// Error used for converting from and into [`crate::ipld::Ipld`].
#[derive(Clone, Debug)]
#[non_exhaustive]
pub enum ConversionError {
    /// Error when the IPLD kind wasn't the one we expected.
    WrongIpldKind {
        /// The expected type.
        expected: IpldKind,
        /// The actual type.
        found: IpldKind,
    },
    /// Error when the given Ipld kind cannot be converted into a certain value type.
    FromIpld {
        /// The IPLD kind trying to convert from.
        from: IpldKind,
        /// The type trying to convert into.
        into: TypeId,
    },
}

impl fmt::Display for ConversionError {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Self::WrongIpldKind { expected, found } => {
                write!(
                    formatter,
                    "kind error: expected {:?} but found {:?}",
                    expected, found
                )
            }
            Self::FromIpld { from, into } => {
                write!(
                    formatter,
                    "conversion error: cannot convert {:?} into {:?}",
                    from, into
                )
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ConversionError {}

impl TryFrom<Ipld> for () {
    type Error = ConversionError;

    fn try_from(ipld: Ipld) -> Result<Self, Self::Error> {
        match ipld {
            Ipld::Null => Ok(()),
            _ => Err(ConversionError::WrongIpldKind {
                expected: IpldKind::Null,
                found: ipld.kind(),
            }),
        }
    }
}

macro_rules! derive_try_from_ipld_option {
    ($enum:ident, $ty:ty) => {
        impl TryFrom<Ipld> for Option<$ty> {
            type Error = ConversionError;

            fn try_from(ipld: Ipld) -> Result<Self, Self::Error> {
                match ipld {
                    Ipld::Null => Ok(None),
                    Ipld::$enum(value) => Ok(Some(value.try_into().map_err(|_| {
                        ConversionError::FromIpld {
                            from: IpldKind::$enum,
                            into: TypeId::of::<$ty>(),
                        }
                    })?)),
                    _ => Err(ConversionError::WrongIpldKind {
                        expected: IpldKind::$enum,
                        found: ipld.kind(),
                    }),
                }
            }
        }
    };
}

macro_rules! derive_try_from_ipld {
    ($enum:ident, $ty:ty) => {
        impl TryFrom<Ipld> for $ty {
            type Error = ConversionError;

            fn try_from(ipld: Ipld) -> Result<Self, Self::Error> {
                match ipld {
                    Ipld::$enum(value) => {
                        Ok(value.try_into().map_err(|_| ConversionError::FromIpld {
                            from: IpldKind::$enum,
                            into: TypeId::of::<$ty>(),
                        })?)
                    }

                    _ => Err(ConversionError::WrongIpldKind {
                        expected: IpldKind::$enum,
                        found: ipld.kind(),
                    }),
                }
            }
        }
    };
}

macro_rules! derive_into_ipld_prim {
    ($enum:ident, $ty:ty, $fn:ident) => {
        impl From<$ty> for Ipld {
            fn from(t: $ty) -> Self {
                Ipld::$enum(t.$fn() as _)
            }
        }
    };
}

macro_rules! derive_into_ipld {
    ($enum:ident, $ty:ty, $($fn:ident),*) => {
        impl From<$ty> for Ipld {
            fn from(t: $ty) -> Self {
                Ipld::$enum(t$(.$fn())*)
            }
        }
    };
}

derive_into_ipld!(Bool, bool, clone);
derive_into_ipld_prim!(Integer, i8, clone);
derive_into_ipld_prim!(Integer, i16, clone);
derive_into_ipld_prim!(Integer, i32, clone);
derive_into_ipld_prim!(Integer, i64, clone);
derive_into_ipld_prim!(Integer, i128, clone);
derive_into_ipld_prim!(Integer, isize, clone);
derive_into_ipld_prim!(Integer, u8, clone);
derive_into_ipld_prim!(Integer, u16, clone);
derive_into_ipld_prim!(Integer, u32, clone);
derive_into_ipld_prim!(Integer, u64, clone);
derive_into_ipld_prim!(Integer, usize, clone);
derive_into_ipld_prim!(Float, f32, clone);
derive_into_ipld_prim!(Float, f64, clone);
derive_into_ipld!(String, String, into);
derive_into_ipld!(String, &str, to_string);
derive_into_ipld!(Bytes, Box<[u8]>, into_vec);
derive_into_ipld!(Bytes, Vec<u8>, into);
derive_into_ipld!(Bytes, &[u8], to_vec);
derive_into_ipld!(List, Vec<Ipld>, into);
derive_into_ipld!(Map, BTreeMap<String, Ipld>, to_owned);
derive_into_ipld!(Link, Cid, clone);
derive_into_ipld!(Link, &Cid, to_owned);

derive_try_from_ipld!(Bool, bool);
derive_try_from_ipld!(Integer, i8);
derive_try_from_ipld!(Integer, i16);
derive_try_from_ipld!(Integer, i32);
derive_try_from_ipld!(Integer, i64);
derive_try_from_ipld!(Integer, i128);
derive_try_from_ipld!(Integer, isize);
derive_try_from_ipld!(Integer, u8);
derive_try_from_ipld!(Integer, u16);
derive_try_from_ipld!(Integer, u32);
derive_try_from_ipld!(Integer, u64);
derive_try_from_ipld!(Integer, u128);
derive_try_from_ipld!(Integer, usize);

//derive_from_ipld!(Float, f32); // User explicit conversion is prefered. Would implicitly lossily convert from f64.

derive_try_from_ipld!(Float, f64);
derive_try_from_ipld!(String, String);
derive_try_from_ipld!(Bytes, Vec<u8>);
derive_try_from_ipld!(List, Vec<Ipld>);
derive_try_from_ipld!(Map, BTreeMap<String, Ipld>);
derive_try_from_ipld!(Link, Cid);

derive_try_from_ipld_option!(Bool, bool);
derive_try_from_ipld_option!(Integer, i8);
derive_try_from_ipld_option!(Integer, i16);
derive_try_from_ipld_option!(Integer, i32);
derive_try_from_ipld_option!(Integer, i64);
derive_try_from_ipld_option!(Integer, i128);
derive_try_from_ipld_option!(Integer, isize);
derive_try_from_ipld_option!(Integer, u8);
derive_try_from_ipld_option!(Integer, u16);
derive_try_from_ipld_option!(Integer, u32);
derive_try_from_ipld_option!(Integer, u64);
derive_try_from_ipld_option!(Integer, u128);
derive_try_from_ipld_option!(Integer, usize);

//derive_from_ipld_option!(Float, f32); // User explicit conversion is prefered. Would implicitly lossily convert from f64.

derive_try_from_ipld_option!(Float, f64);
derive_try_from_ipld_option!(String, String);
derive_try_from_ipld_option!(Bytes, Vec<u8>);
derive_try_from_ipld_option!(List, Vec<Ipld>);
derive_try_from_ipld_option!(Map, BTreeMap<String, Ipld>);
derive_try_from_ipld_option!(Link, Cid);

#[cfg(test)]
mod tests {
    use alloc::{collections::BTreeMap, string::String, vec, vec::Vec};

    use cid::Cid;

    use crate::ipld::Ipld;

    #[test]
    #[should_panic]
    fn try_into_wrong_type() {
        let _boolean: bool = Ipld::Integer(u8::MAX as i128).try_into().unwrap();
    }

    #[test]
    #[should_panic]
    fn try_into_wrong_range() {
        let int: u128 = Ipld::Integer(-1i128).try_into().unwrap();
        assert_eq!(int, u128::MIN);
    }

    #[test]
    fn try_into_bool() {
        let boolean: bool = Ipld::Bool(true).try_into().unwrap();
        assert!(boolean);

        let boolean: Option<bool> = Ipld::Null.try_into().unwrap();
        assert_eq!(boolean, Option::None)
    }

    #[test]
    fn try_into_ints() {
        let int: u8 = Ipld::Integer(u8::MAX as i128).try_into().unwrap();
        assert_eq!(int, u8::MAX);

        let int: u16 = Ipld::Integer(u16::MAX as i128).try_into().unwrap();
        assert_eq!(int, u16::MAX);

        let int: u32 = Ipld::Integer(u32::MAX as i128).try_into().unwrap();
        assert_eq!(int, u32::MAX);

        let int: u64 = Ipld::Integer(u64::MAX as i128).try_into().unwrap();
        assert_eq!(int, u64::MAX);

        let int: usize = Ipld::Integer(usize::MAX as i128).try_into().unwrap();
        assert_eq!(int, usize::MAX);

        let int: u128 = Ipld::Integer(i128::MAX).try_into().unwrap();
        assert_eq!(int, i128::MAX as u128);

        let int: i8 = Ipld::Integer(i8::MIN as i128).try_into().unwrap();
        assert_eq!(int, i8::MIN);

        let int: i16 = Ipld::Integer(i16::MIN as i128).try_into().unwrap();
        assert_eq!(int, i16::MIN);

        let int: i32 = Ipld::Integer(i32::MIN as i128).try_into().unwrap();
        assert_eq!(int, i32::MIN);

        let int: i64 = Ipld::Integer(i64::MIN as i128).try_into().unwrap();
        assert_eq!(int, i64::MIN);

        let int: isize = Ipld::Integer(isize::MIN as i128).try_into().unwrap();
        assert_eq!(int, isize::MIN);

        let int: i128 = Ipld::Integer(i128::MIN).try_into().unwrap();
        assert_eq!(int, i128::MIN);

        let int: Option<i32> = Ipld::Null.try_into().unwrap();
        assert_eq!(int, Option::None)
    }

    #[test]
    fn try_into_floats() {
        /* let float: f32 = Ipld::Float(f32::MAX as f64).try_into().unwrap();
        assert_eq!(float, f32::MAX); */

        let float: f64 = Ipld::Float(f64::MAX).try_into().unwrap();
        assert_eq!(float, f64::MAX);

        let float: Option<f64> = Ipld::Null.try_into().unwrap();
        assert_eq!(float, Option::None)
    }

    #[test]
    fn try_into_string() {
        let lyrics: String = "I'm blue babedi babeda".into();
        let string: String = Ipld::String(lyrics.clone()).try_into().unwrap();
        assert_eq!(string, lyrics);

        let option: Option<String> = Ipld::Null.try_into().unwrap();
        assert_eq!(option, Option::None)
    }

    #[test]
    fn try_into_vec() {
        let data = vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        let bytes: Vec<u8> = Ipld::Bytes(data.clone()).try_into().unwrap();
        assert_eq!(bytes, data);

        let option: Option<Vec<u8>> = Ipld::Null.try_into().unwrap();
        assert_eq!(option, Option::None)
    }

    #[test]
    fn try_into_list() {
        let ints = vec![Ipld::Integer(0), Ipld::Integer(1), Ipld::Integer(2)];
        let list: Vec<Ipld> = Ipld::List(ints.clone()).try_into().unwrap();
        assert_eq!(ints, list);

        let option: Option<Vec<Ipld>> = Ipld::Null.try_into().unwrap();
        assert_eq!(option, Option::None)
    }

    #[test]
    fn try_into_map() {
        let mut numbs = BTreeMap::new();
        numbs.insert("zero".into(), Ipld::Integer(0));
        numbs.insert("one".into(), Ipld::Integer(1));
        numbs.insert("two".into(), Ipld::Integer(2));
        let map: BTreeMap<String, Ipld> = Ipld::Map(numbs.clone()).try_into().unwrap();
        assert_eq!(numbs, map);

        let option: Option<BTreeMap<String, Ipld>> = Ipld::Null.try_into().unwrap();
        assert_eq!(option, Option::None)
    }

    #[test]
    fn try_into_cid() {
        let cid = Cid::default();
        let link: Cid = Ipld::Link(cid).try_into().unwrap();
        assert_eq!(cid, link);

        let option: Option<Cid> = Ipld::Null.try_into().unwrap();
        assert_eq!(option, Option::None)
    }
}
