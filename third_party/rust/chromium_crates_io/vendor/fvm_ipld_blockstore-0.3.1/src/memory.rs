// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use std::cell::RefCell;
use std::collections::HashMap;

use anyhow::Result;
use cid::Cid;

use super::Blockstore;

#[derive(Debug, Default, Clone)]
pub struct MemoryBlockstore {
    blocks: RefCell<HashMap<Cid, Vec<u8>>>,
}

impl MemoryBlockstore {
    pub fn new() -> Self {
        Self::default()
    }

    /// Copy all blocks from this blockstore into the target blockstore.
    pub fn copy_to(&self, other: &impl Blockstore) -> Result<()> {
        other.put_many_keyed(self.blocks.borrow().iter().map(|(&k, v)| (k, v)))
    }
}

impl Blockstore for MemoryBlockstore {
    fn has(&self, k: &Cid) -> Result<bool> {
        Ok(self.blocks.borrow().contains_key(k))
    }

    fn get(&self, k: &Cid) -> Result<Option<Vec<u8>>> {
        Ok(self.blocks.borrow().get(k).cloned())
    }

    fn put_keyed(&self, k: &Cid, block: &[u8]) -> Result<()> {
        self.blocks.borrow_mut().insert(*k, block.into());
        Ok(())
    }
}
