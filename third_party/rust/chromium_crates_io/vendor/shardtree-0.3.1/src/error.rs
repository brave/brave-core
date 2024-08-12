//! Error types for this crate.

use std::fmt;
use std::ops::Range;

use incrementalmerkletree::{Address, Position};

/// The error type for operations on a [`ShardTree`].
///
/// The parameter `S` is set to [`ShardStore::Error`].
///
/// [`ShardTree`]: crate::ShardTree
/// [`ShardStore::Error`]: crate::ShardStore::Error
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ShardTreeError<S> {
    Query(QueryError),
    Insert(InsertionError),
    Storage(S),
}

impl<S> From<QueryError> for ShardTreeError<S> {
    fn from(err: QueryError) -> Self {
        ShardTreeError::Query(err)
    }
}

impl<S> From<InsertionError> for ShardTreeError<S> {
    fn from(err: InsertionError) -> Self {
        ShardTreeError::Insert(err)
    }
}

impl<S: fmt::Display> fmt::Display for ShardTreeError<S> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &self {
            ShardTreeError::Query(q) => q.fmt(f),
            ShardTreeError::Insert(i) => i.fmt(f),
            ShardTreeError::Storage(s) => {
                write!(
                    f,
                    "An error occurred persisting or retrieving tree data: {}",
                    s
                )
            }
        }
    }
}

impl<SE> std::error::Error for ShardTreeError<SE>
where
    SE: std::error::Error + 'static,
{
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match &self {
            ShardTreeError::Storage(e) => Some(e),
            _ => None,
        }
    }
}

/// Errors which can occur when inserting values into a subtree.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum InsertionError {
    /// The caller attempted to insert a subtree into a tree that does not contain
    /// the subtree's root address.
    NotContained(Address),
    /// The start of the range of positions provided for insertion is not included
    /// in the range of positions within this subtree.
    OutOfRange(Position, Range<Position>),
    /// An existing root hash conflicts with the root hash of a node being inserted.
    Conflict(Address),
    /// An out-of-order checkpoint was detected
    ///
    /// Checkpoint identifiers must be in nondecreasing order relative to tree positions.
    CheckpointOutOfOrder,
    /// An append operation has exceeded the capacity of the tree.
    TreeFull,
    /// An input data structure had malformed data when attempting to insert a value
    /// at the given address
    InputMalformed(Address),
    // The caller attempted to mark the empty tree state as corresponding to the state
    // for a spendable note.
    MarkedRetentionInvalid,
}

impl fmt::Display for InsertionError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &self {
            InsertionError::NotContained(addr) => {
                write!(f, "Tree does not contain a root at address {:?}", addr)
            }
            InsertionError::OutOfRange(p, r) => {
                write!(
                    f,
                    "Attempted insertion point {:?} is not in range {:?}",
                    p, r
                )
            }
            InsertionError::Conflict(addr) => write!(
                f,
                "Inserted root conflicts with existing root at address {:?}",
                addr
            ),
            InsertionError::CheckpointOutOfOrder => {
                write!(f, "Cannot append out-of-order checkpoint identifier.")
            }
            InsertionError::TreeFull => write!(f, "Note commitment tree is full."),
            InsertionError::InputMalformed(addr) => {
                write!(f, "Input malformed for insertion at address {:?}", addr)
            }
            InsertionError::MarkedRetentionInvalid => {
                write!(f, "Cannot use `Marked` retention for the empty tree.")
            }
        }
    }
}

impl std::error::Error for InsertionError {}

/// Errors which can occur when querying a [`ShardTree`].
///
/// [`ShardTree`]: crate::ShardTree
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum QueryError {
    /// The caller attempted to query the value at an address within a tree that does not contain
    /// that address.
    NotContained(Address),
    /// A leaf required by a given checkpoint has been pruned, or is otherwise not accessible in
    /// the tree.
    CheckpointPruned,
    /// It is not possible to compute a root for one or more subtrees because they contain
    /// [`Node::Nil`] values at positions that cannot be replaced with default hashes.
    ///
    /// [`Node::Nil`]: crate::Node::Nil
    TreeIncomplete(Vec<Address>),
}

impl fmt::Display for QueryError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match &self {
            QueryError::NotContained(addr) => {
                write!(f, "Tree does not contain a root at address {:?}", addr)
            }
            QueryError::CheckpointPruned => {
                write!(
                    f,
                    "The leaf corresponding to the requested checkpoint is not present in the tree."
                )
            }
            QueryError::TreeIncomplete(addrs) => {
                write!(
                    f,
                    "Unable to compute root; missing values for nodes {:?}",
                    addrs
                )
            }
        }
    }
}

impl std::error::Error for QueryError {}
