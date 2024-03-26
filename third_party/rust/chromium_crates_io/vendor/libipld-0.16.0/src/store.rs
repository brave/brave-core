//! Store traits.
//!
//! ## Aliases
//! An alias is a named root of a dag. When a root is aliased, none of the leaves of the dag
//! pointed to by the root will be collected by gc. However, a root being aliased does not
//! mean that the dag must be complete.
//!
//! ## Temporary pin
//! A temporary pin is an unnamed set of roots of a dag, that is just for the purpose of protecting
//! blocks from gc while a large tree is constructed. While an alias maps a single name to a
//! single root, a temporary alias can be assigned to an arbitrary number of blocks before the
//! dag is finished.
//!
//! ## Garbage collection (GC)
//! GC refers to the process of removing unaliased blocks. When it runs is implementation defined.
//! However it is intended to run only when the configured size is exceeded at when it will start
//! incrementally deleting unaliased blocks until the size target is no longer exceeded. It is
//! implementation defined in which order unaliased blocks get removed.
use crate::codec::Codec;
use crate::multihash::MultihashDigest;

/// The store parameters.
pub trait StoreParams: std::fmt::Debug + Clone + Send + Sync + Unpin + 'static {
    /// The multihash type of the store.
    type Hashes: MultihashDigest<64>;
    /// The codec type of the store.
    type Codecs: Codec;
    /// The maximum block size supported by the store.
    const MAX_BLOCK_SIZE: usize;
}

/// Default store parameters.
#[derive(Clone, Debug, Default)]
pub struct DefaultParams;

impl StoreParams for DefaultParams {
    const MAX_BLOCK_SIZE: usize = 1_048_576;
    type Codecs = crate::IpldCodec;
    type Hashes = crate::multihash::Code;
}
