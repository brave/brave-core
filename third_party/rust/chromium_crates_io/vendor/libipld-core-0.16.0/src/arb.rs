//! Ipld representation.
use alloc::{boxed::Box, string::String, vec::Vec};

use crate::{cid::Cid, ipld::Ipld};
use quickcheck::empty_shrinker;
use quickcheck::Arbitrary;

impl quickcheck::Arbitrary for Ipld {
    fn arbitrary(g: &mut quickcheck::Gen) -> Self {
        Self::arbitrary_ipld(g, &mut g.size())
    }

    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        match self {
            Ipld::Null => empty_shrinker(),
            Ipld::Bool(v) => Box::new(v.shrink().map(Ipld::Bool)),
            Ipld::Integer(v) => Box::new(v.shrink().map(Ipld::Integer)),
            Ipld::Float(v) => Box::new(v.shrink().map(Ipld::Float)),
            Ipld::String(v) => Box::new(v.shrink().map(Ipld::String)),
            Ipld::Bytes(v) => Box::new(v.shrink().map(Ipld::Bytes)),
            Ipld::List(v) => Box::new(v.shrink().map(Ipld::List)),
            Ipld::Map(v) => Box::new(v.shrink().map(Ipld::Map)),
            Ipld::Link(v) => Box::new(v.shrink().map(Ipld::Link)),
        }
    }
}

impl Ipld {
    /// Special version on `arbitrary` to battle possible recursion
    fn arbitrary_ipld(g: &mut quickcheck::Gen, size: &mut usize) -> Self {
        if *size == 0 {
            return Ipld::Null;
        }
        *size -= 1;
        let index = usize::arbitrary(g) % 9;
        match index {
            0 => Ipld::Null,
            1 => Ipld::Bool(bool::arbitrary(g)),
            2 => Ipld::Integer(i128::arbitrary(g)),
            3 => Ipld::Float(f64::arbitrary(g)),
            4 => Ipld::String(String::arbitrary(g)),
            5 => Ipld::Bytes(Vec::arbitrary(g)),
            6 => Ipld::List(
                (0..Self::arbitrary_size(g, size))
                    .map(|_| Self::arbitrary_ipld(g, size))
                    .collect(),
            ),
            7 => Ipld::Map(
                (0..Self::arbitrary_size(g, size))
                    .map(|_| (String::arbitrary(g), Self::arbitrary_ipld(g, size)))
                    .collect(),
            ),
            8 => Ipld::Link(Cid::arbitrary(g)),
            // unreachable due to the fact that
            // we know that the index is always < 9
            _ => unreachable!(),
        }
    }

    fn arbitrary_size(g: &mut quickcheck::Gen, size: &mut usize) -> usize {
        if *size == 0 {
            return 0;
        }
        usize::arbitrary(g) % *size
    }
}
