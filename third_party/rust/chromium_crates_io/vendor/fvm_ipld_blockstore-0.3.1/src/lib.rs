// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use std::rc::Rc;
use std::sync::Arc;

use anyhow::Result;
use cid::Cid;

pub mod tracking;

mod memory;
pub use memory::MemoryBlockstore;

mod block;
pub use block::*;

/// An IPLD blockstore suitable for injection into the FVM.
///
/// The cgo blockstore adapter implements this trait.
pub trait Blockstore {
    /// Gets the block from the blockstore.
    fn get(&self, k: &Cid) -> Result<Option<Vec<u8>>>;

    /// Put a block with a pre-computed cid.
    ///
    /// If you don't yet know the CID, use put. Some blockstores will re-compute the CID internally
    /// even if you provide it.
    ///
    /// If you _do_ already know the CID, use this method as some blockstores _won't_ recompute it.
    fn put_keyed(&self, k: &Cid, block: &[u8]) -> Result<()>;

    /// Checks if the blockstore has the specified block.
    fn has(&self, k: &Cid) -> Result<bool> {
        Ok(self.get(k)?.is_some())
    }

    /// Puts the block into the blockstore, computing the hash with the specified multicodec.
    ///
    /// By default, this defers to put.
    fn put<D>(&self, mh_code: multihash_codetable::Code, block: &Block<D>) -> Result<Cid>
    where
        Self: Sized,
        D: AsRef<[u8]>,
    {
        let k = block.cid(mh_code);
        self.put_keyed(&k, block.as_ref())?;
        Ok(k)
    }

    /// Bulk put blocks into the blockstore.
    ///
    ///
    /// ```rust
    /// use multihash_codetable::Code::Blake2b256;
    /// use fvm_ipld_blockstore::{Blockstore, MemoryBlockstore, Block};
    ///
    /// let bs = MemoryBlockstore::default();
    /// let blocks = vec![Block::new(0x55, vec![0, 1, 2])];
    /// bs.put_many(blocks.iter().map(|b| (Blake2b256, b.into()))).unwrap();
    /// ```
    fn put_many<D, I>(&self, blocks: I) -> Result<()>
    where
        Self: Sized,
        D: AsRef<[u8]>,
        I: IntoIterator<Item = (multihash_codetable::Code, Block<D>)>,
    {
        self.put_many_keyed(blocks.into_iter().map(|(mc, b)| (b.cid(mc), b)))?;
        Ok(())
    }

    /// Bulk-put pre-keyed blocks into the blockstore.
    ///
    /// By default, this defers to put_keyed.
    fn put_many_keyed<D, I>(&self, blocks: I) -> Result<()>
    where
        Self: Sized,
        D: AsRef<[u8]>,
        I: IntoIterator<Item = (Cid, D)>,
    {
        for (c, b) in blocks {
            self.put_keyed(&c, b.as_ref())?
        }
        Ok(())
    }
}

pub trait Buffered: Blockstore {
    fn flush(&self, root: &Cid) -> Result<()>;
}

macro_rules! impl_blockstore {
    ($($typ:ty),+) => {
        $(
            impl<BS> Blockstore for $typ where
            BS: Blockstore, {
                fn get(&self, k: &Cid) -> Result<Option<Vec<u8>>> {
                    (**self).get(k)
                }

                fn put_keyed(&self, k: &Cid, block: &[u8]) -> Result<()> {
                    (**self).put_keyed(k, block)
                }

                fn has(&self, k: &Cid) -> Result<bool> {
                    (**self).has(k)
                }

                fn put<D>(&self, mh_code: multihash_codetable::Code, block: &Block<D>) -> Result<Cid>
                where
                    Self: Sized,
                    D: AsRef<[u8]>,
                {
                    (**self).put(mh_code, block)
                }

                fn put_many<D, I>(&self, blocks: I) -> Result<()>
                where
                    Self: Sized,
                    D: AsRef<[u8]>,
                    I: IntoIterator<Item = (multihash_codetable::Code, Block<D>)>,
                {
                    (**self).put_many(blocks)
                }

                fn put_many_keyed<D, I>(&self, blocks: I) -> Result<()>
                where
                    Self: Sized,
                    D: AsRef<[u8]>,
                    I: IntoIterator<Item = (Cid, D)>,
                {
                    (**self).put_many_keyed(blocks)
                }
            }
        )+
    }
}

impl_blockstore!(Arc<BS>, Rc<BS>, &BS);
