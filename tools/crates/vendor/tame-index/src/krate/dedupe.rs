//! Contains helpers for deduplicating dependencies and features for a single
//! crate during parsing as each individual version of a crate tends to be mostly
//! the same
//!
//! Copied from <https://github.com/frewsxcv/rust-crates-index/blob/master/src/dedupe.rs>

use crate::krate::IndexDependency;
use std::{
    hash::{BuildHasherDefault, Hash, Hasher},
    sync::Arc,
};

type XxSet<K> = std::collections::HashSet<K, BuildHasherDefault<twox_hash::XxHash64>>;
use super::FeatureMap;

/// Many crate versions have the same features and dependencies
#[derive(Default)]
pub(crate) struct DedupeContext {
    features: XxSet<HashedFeatureMap>,
    deps: XxSet<Arc<[IndexDependency]>>,
}

impl DedupeContext {
    pub(crate) fn features(&mut self, features: &mut Arc<FeatureMap>) {
        let features_to_dedupe = HashedFeatureMap::new(Arc::clone(features));
        if let Some(has_feats) = self.features.get(&features_to_dedupe) {
            *features = Arc::clone(&has_feats.map);
        } else {
            // keeps peak memory low (must clear, remove is leaving tombstones)
            if self.features.len() > 16 * 1024 {
                self.features.clear();
            }
            self.features.insert(features_to_dedupe);
        }
    }

    pub(crate) fn deps(&mut self, deps: &mut Arc<[IndexDependency]>) {
        if let Some(has_deps) = self.deps.get(&*deps) {
            *deps = Arc::clone(has_deps);
        } else {
            // keeps peak memory low (must clear, remove is leaving tombstones)
            if self.deps.len() > 16 * 1024 {
                self.deps.clear();
            }
            self.deps.insert(Arc::clone(deps));
        }
    }
}

/// Newtype that caches hash of the hashmap (the default hashmap has a random order of the keys, so it's not cheap to hash)
#[derive(PartialEq, Eq)]
pub struct HashedFeatureMap {
    pub map: Arc<FeatureMap>,
    hash: u64,
}

impl Hash for HashedFeatureMap {
    #[inline]
    fn hash<H>(&self, hasher: &mut H)
    where
        H: Hasher,
    {
        hasher.write_u64(self.hash);
    }
}

impl HashedFeatureMap {
    #[inline]
    pub(crate) fn new(map: Arc<FeatureMap>) -> Self {
        let mut hash = 0;
        for (k, v) in map.iter() {
            let mut hasher = twox_hash::XxHash64::default();
            k.hash(&mut hasher);
            v.hash(&mut hasher);
            hash ^= hasher.finish(); // XOR makes it order-independent
        }
        Self { map, hash }
    }
}
