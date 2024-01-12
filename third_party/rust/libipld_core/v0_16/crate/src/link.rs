//! Typed cid.
use core::{
    cmp::Ordering,
    fmt,
    hash::{Hash, Hasher},
    marker::PhantomData,
    ops::Deref,
};

use crate::cid::Cid;
use crate::codec::{Codec, Decode, Encode};
use crate::error::Result;
use crate::io::{Read, Seek, Write};

/// Typed cid.
#[derive(Debug)]
pub struct Link<T> {
    cid: Cid,
    _marker: PhantomData<T>,
}

impl<T> Link<T> {
    /// Creates a new `Link`.
    pub fn new(cid: Cid) -> Self {
        Self {
            cid,
            _marker: PhantomData,
        }
    }

    /// Returns a reference to the cid.
    pub fn cid(&self) -> &Cid {
        &self.cid
    }
}

impl<T> fmt::Display for Link<T> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        self.cid.fmt(f)
    }
}

impl<T> Clone for Link<T> {
    fn clone(&self) -> Self {
        Self {
            cid: self.cid,
            _marker: self._marker,
        }
    }
}

impl<T> Copy for Link<T> {}

impl<T> PartialEq for Link<T> {
    fn eq(&self, other: &Self) -> bool {
        self.cid.eq(other.cid())
    }
}

impl<T> Eq for Link<T> {}

impl<T> PartialOrd for Link<T> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cid.cmp(other.cid()))
    }
}

impl<T> Ord for Link<T> {
    fn cmp(&self, other: &Self) -> Ordering {
        self.cid.cmp(other.cid())
    }
}

impl<T> Hash for Link<T> {
    fn hash<H: Hasher>(&self, hasher: &mut H) {
        Hash::hash(self.cid(), hasher)
    }
}

impl<C: Codec, T> Encode<C> for Link<T>
where
    Cid: Encode<C>,
{
    fn encode<W: Write>(&self, c: C, w: &mut W) -> Result<()> {
        self.cid().encode(c, w)
    }
}

impl<C: Codec, T> Decode<C> for Link<T>
where
    Cid: Decode<C>,
{
    fn decode<R: Read + Seek>(c: C, r: &mut R) -> Result<Self> {
        Ok(Self::new(Cid::decode(c, r)?))
    }
}

impl<T> Deref for Link<T> {
    type Target = Cid;

    fn deref(&self) -> &Self::Target {
        self.cid()
    }
}

impl<T> AsRef<Cid> for Link<T> {
    fn as_ref(&self) -> &Cid {
        self.cid()
    }
}

impl<T> From<Cid> for Link<T> {
    fn from(cid: Cid) -> Self {
        Self::new(cid)
    }
}
