// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
//#![cfg(feature = "tracking")]

use std::cell::RefCell;

use anyhow::Result;
use cid::multihash::{self, Code};
use cid::Cid;

use super::{Block, Blockstore};

/// Stats for a [TrackingBlockstore] this indicates the amount of read and written data
/// to the wrapped store.
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub struct BSStats {
    /// Number of reads
    pub r: usize,
    /// Number of writes
    pub w: usize,
    /// Bytes Read
    pub br: usize,
    /// Bytes Written
    pub bw: usize,
}

/// Wrapper around `Blockstore` to tracking reads and writes for verification.
/// This struct should only be used for testing.
#[derive(Debug)]
pub struct TrackingBlockstore<BS> {
    base: BS,
    pub stats: RefCell<BSStats>,
}

impl<BS> TrackingBlockstore<BS>
where
    BS: Blockstore,
{
    pub fn new(base: BS) -> Self {
        Self {
            base,
            stats: Default::default(),
        }
    }
}

impl<BS> Blockstore for TrackingBlockstore<BS>
where
    BS: Blockstore,
{
    fn get(&self, cid: &Cid) -> Result<Option<Vec<u8>>> {
        let mut stats = self.stats.borrow_mut();
        stats.r += 1;
        let bytes = self.base.get(cid)?;
        if let Some(bytes) = &bytes {
            stats.br += bytes.len();
        }
        Ok(bytes)
    }
    fn has(&self, cid: &Cid) -> Result<bool> {
        self.stats.borrow_mut().r += 1;
        self.base.has(cid)
    }

    fn put<D>(&self, code: Code, block: &Block<D>) -> Result<Cid>
    where
        D: AsRef<[u8]>,
    {
        let mut stats = self.stats.borrow_mut();
        stats.w += 1;
        stats.bw += block.as_ref().len();
        self.base.put(code, block)
    }

    fn put_keyed(&self, k: &Cid, block: &[u8]) -> Result<()> {
        let mut stats = self.stats.borrow_mut();
        stats.w += 1;
        stats.bw += block.len();
        self.base.put_keyed(k, block)
    }

    fn put_many<D, I>(&self, blocks: I) -> Result<()>
    where
        Self: Sized,
        D: AsRef<[u8]>,
        I: IntoIterator<Item = (multihash::Code, Block<D>)>,
    {
        let mut stats = self.stats.borrow_mut();
        self.base.put_many(blocks.into_iter().inspect(|(_, b)| {
            stats.w += 1;
            stats.bw += b.as_ref().len();
        }))?;
        Ok(())
    }

    fn put_many_keyed<D, I>(&self, blocks: I) -> Result<()>
    where
        Self: Sized,
        D: AsRef<[u8]>,
        I: IntoIterator<Item = (Cid, D)>,
    {
        let mut stats = self.stats.borrow_mut();
        self.base
            .put_many_keyed(blocks.into_iter().inspect(|(_, b)| {
                stats.w += 1;
                stats.bw += b.as_ref().len();
            }))?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{Block, MemoryBlockstore};

    #[test]
    fn basic_tracking_store() {
        let mem = MemoryBlockstore::default();
        let tr_store = TrackingBlockstore::new(&mem);
        assert_eq!(*tr_store.stats.borrow(), BSStats::default());

        let block = Block::new(0x55, &b"foobar"[..]);
        tr_store.get(&block.cid(Code::Blake2b256)).unwrap();
        assert_eq!(
            *tr_store.stats.borrow(),
            BSStats {
                r: 1,
                ..Default::default()
            }
        );

        let put_cid = tr_store.put(Code::Blake2b256, &block).unwrap();
        assert_eq!(tr_store.get(&put_cid).unwrap().as_deref(), Some(block.data));
        assert_eq!(
            *tr_store.stats.borrow(),
            BSStats {
                r: 2,
                br: block.len(),
                w: 1,
                bw: block.len(),
            }
        );
    }
}
