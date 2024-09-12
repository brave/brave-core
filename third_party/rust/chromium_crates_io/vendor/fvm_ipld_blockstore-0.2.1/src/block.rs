use std::fmt;

// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use cid::multihash::{self, MultihashDigest};
use cid::Cid;

/// Block represents a typed (i.e., with codec) IPLD block.
#[derive(Copy, Clone)]
pub struct Block<D>
where
    D: AsRef<[u8]> + ?Sized,
{
    pub codec: u64,
    pub data: D,
}

impl<D> Block<D>
where
    D: AsRef<[u8]> + ?Sized,
{
    pub const fn new(codec: u64, data: D) -> Self
    where
        Self: Sized,
        D: Sized,
    {
        Self { codec, data }
    }

    pub fn cid(&self, mh_code: multihash::Code) -> Cid {
        Cid::new_v1(self.codec, mh_code.digest(self.data.as_ref()))
    }

    #[allow(clippy::len_without_is_empty)]
    pub fn len(&self) -> usize {
        self.data.as_ref().len()
    }
}

// Manually implement PartialEq/Eq so we can compare across blocks with different backing buffers.
impl<D1, D2> PartialEq<Block<D2>> for Block<D1>
where
    D1: AsRef<[u8]>,
    D2: AsRef<[u8]>,
{
    fn eq(&self, other: &Block<D2>) -> bool {
        self.codec == other.codec && self.data.as_ref() == other.data.as_ref()
    }
}
impl<D> Eq for Block<D> where D: AsRef<[u8]> {}

// Manually implement debug so we get the same result regardless of the backing buff for data.
impl<D> fmt::Debug for Block<D>
where
    D: AsRef<[u8]>,
{
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Block")
            .field("codec", &self.codec)
            .field("data", &self.data.as_ref())
            .finish()
    }
}

impl<D> AsRef<[u8]> for Block<D>
where
    D: AsRef<[u8]>,
{
    fn as_ref(&self) -> &[u8] {
        self.data.as_ref()
    }
}

impl<'a, D> From<&'a Block<D>> for Block<&'a [u8]>
where
    D: AsRef<[u8]>,
{
    fn from(b: &'a Block<D>) -> Self {
        Block {
            codec: b.codec,
            data: b.data.as_ref(),
        }
    }
}
