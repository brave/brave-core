use std::convert::TryFrom;

use assert_matches::assert_matches;
use proptest::bool::weighted;
use proptest::collection::vec;
use proptest::prelude::*;
use proptest::sample::select;

use incrementalmerkletree::{testing, Hashable};

use super::*;
use crate::store::{memory::MemoryShardStore, ShardStore};

pub fn arb_retention_flags() -> impl Strategy<Value = RetentionFlags> + Clone {
    select(vec![
        RetentionFlags::EPHEMERAL,
        RetentionFlags::CHECKPOINT,
        RetentionFlags::MARKED,
        RetentionFlags::MARKED | RetentionFlags::CHECKPOINT,
    ])
}

pub fn arb_tree<A: Strategy + Clone + 'static, V: Strategy + 'static>(
    arb_annotation: A,
    arb_leaf: V,
    depth: u32,
    size: u32,
) -> impl Strategy<Value = Tree<A::Value, V::Value>> + Clone
where
    A::Value: Clone + 'static,
    V::Value: Clone + 'static,
{
    let leaf = prop_oneof![
        Just(Tree(Node::Nil)),
        arb_leaf.prop_map(|value| Tree(Node::Leaf { value }))
    ];

    leaf.prop_recursive(depth, size, 2, move |inner| {
        (arb_annotation.clone(), inner.clone(), inner).prop_map(|(ann, left, right)| {
            Tree(if left.is_nil() && right.is_nil() {
                Node::Nil
            } else {
                Node::Parent {
                    ann,
                    left: Arc::new(left),
                    right: Arc::new(right),
                }
            })
        })
    })
}

pub fn arb_prunable_tree<H: Strategy + Clone + 'static>(
    arb_leaf: H,
    depth: u32,
    size: u32,
) -> impl Strategy<Value = PrunableTree<H::Value>> + Clone
where
    H::Value: Clone + 'static,
{
    arb_tree(
        proptest::option::of(arb_leaf.clone().prop_map(Arc::new)),
        (arb_leaf, arb_retention_flags()),
        depth,
        size,
    )
}

/// Constructs a random sequence of leaves that form a tree of size up to 2^6.
pub fn arb_leaves<H: Strategy + Clone>(
    arb_leaf: H,
) -> impl Strategy<Value = Vec<(H::Value, Retention<usize>)>>
where
    H::Value: Hashable + Clone + PartialEq,
{
    vec(
        (arb_leaf, weighted(0.1), weighted(0.2)),
        0..=(2usize.pow(6)),
    )
    .prop_map(|leaves| {
        leaves
            .into_iter()
            .enumerate()
            .map(|(id, (leaf, is_marked, is_checkpoint))| {
                (
                    leaf,
                    match (is_checkpoint, is_marked) {
                        (false, false) => Retention::Ephemeral,
                        (true, is_marked) => Retention::Checkpoint { id, is_marked },
                        (false, true) => Retention::Marked,
                    },
                )
            })
            .collect()
    })
}

/// Constructs a random shardtree of size up to 2^6 with shards of size 2^3. Returns the tree,
/// along with vectors of the checkpoint and mark positions.
pub fn arb_shardtree<H: Strategy + Clone>(
    arb_leaf: H,
) -> impl Strategy<
    Value = (
        ShardTree<MemoryShardStore<H::Value, usize>, 6, 3>,
        Vec<Position>,
        Vec<Position>,
    ),
>
where
    H::Value: Hashable + Clone + PartialEq,
{
    arb_leaves(arb_leaf).prop_map(|leaves| {
        let mut tree = ShardTree::new(MemoryShardStore::empty(), 10);
        let mut checkpoint_positions = vec![];
        let mut marked_positions = vec![];
        tree.batch_insert(
            Position::from(0),
            leaves
                .into_iter()
                .enumerate()
                .map(|(id, (leaf, retention))| {
                    let pos = Position::try_from(id).unwrap();
                    match retention {
                        Retention::Ephemeral => (),
                        Retention::Checkpoint { is_marked, .. } => {
                            checkpoint_positions.push(pos);
                            if is_marked {
                                marked_positions.push(pos);
                            }
                        }
                        Retention::Marked => marked_positions.push(pos),
                    }
                    (leaf, retention)
                }),
        )
        .unwrap();
        (tree, checkpoint_positions, marked_positions)
    })
}

pub fn arb_char_str() -> impl Strategy<Value = String> + Clone {
    let chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    (0usize..chars.len()).prop_map(move |i| chars.get(i..=i).unwrap().to_string())
}

impl<
        H: Hashable + Ord + Clone + core::fmt::Debug,
        C: Clone + Ord + core::fmt::Debug,
        S: ShardStore<H = H, CheckpointId = C>,
        const DEPTH: u8,
        const SHARD_HEIGHT: u8,
    > testing::Tree<H, C> for ShardTree<S, DEPTH, SHARD_HEIGHT>
{
    fn depth(&self) -> u8 {
        DEPTH
    }

    fn append(&mut self, value: H, retention: Retention<C>) -> bool {
        match ShardTree::append(self, value, retention) {
            Ok(_) => true,
            Err(ShardTreeError::Insert(InsertionError::TreeFull)) => false,
            Err(other) => panic!("append failed due to error: {:?}", other),
        }
    }

    fn current_position(&self) -> Option<Position> {
        match ShardTree::max_leaf_position(self, 0) {
            Ok(v) => v,
            Err(err) => panic!("current position query failed: {:?}", err),
        }
    }

    fn get_marked_leaf(&self, position: Position) -> Option<H> {
        match ShardTree::get_marked_leaf(self, position) {
            Ok(v) => v,
            Err(err) => panic!("marked leaf query failed: {:?}", err),
        }
    }

    fn marked_positions(&self) -> BTreeSet<Position> {
        match ShardTree::marked_positions(self) {
            Ok(v) => v,
            Err(err) => panic!("marked positions query failed: {:?}", err),
        }
    }

    fn root(&self, checkpoint_depth: usize) -> Option<H> {
        match ShardTree::root_at_checkpoint_depth(self, checkpoint_depth) {
            Ok(v) => Some(v),
            Err(err) => panic!("root computation failed: {:?}", err),
        }
    }

    fn witness(&self, position: Position, checkpoint_depth: usize) -> Option<Vec<H>> {
        match ShardTree::witness_at_checkpoint_depth(self, position, checkpoint_depth) {
            Ok(p) => Some(p.path_elems().to_vec()),
            Err(ShardTreeError::Query(
                QueryError::NotContained(_)
                | QueryError::TreeIncomplete(_)
                | QueryError::CheckpointPruned,
            )) => None,
            Err(err) => panic!("witness computation failed: {:?}", err),
        }
    }

    fn remove_mark(&mut self, position: Position) -> bool {
        let max_checkpoint = self
            .store
            .max_checkpoint_id()
            .unwrap_or_else(|err| panic!("checkpoint retrieval failed: {:?}", err));

        match ShardTree::remove_mark(self, position, max_checkpoint.as_ref()) {
            Ok(result) => result,
            Err(err) => panic!("mark removal failed: {:?}", err),
        }
    }

    fn checkpoint(&mut self, checkpoint_id: C) -> bool {
        ShardTree::checkpoint(self, checkpoint_id).unwrap()
    }

    fn rewind(&mut self) -> bool {
        ShardTree::truncate_to_depth(self, 1).unwrap()
    }
}

pub fn check_shardtree_insertion<
    E: Debug,
    S: ShardStore<H = String, CheckpointId = u32, Error = E>,
>(
    mut tree: ShardTree<S, 4, 3>,
) {
    assert_matches!(
        tree.batch_insert(
            Position::from(1),
            vec![
                ("b".to_string(), Retention::Checkpoint { id: 1, is_marked: false }),
                ("c".to_string(), Retention::Ephemeral),
                ("d".to_string(), Retention::Marked),
            ].into_iter()
        ),
        Ok(Some((pos, incomplete))) if
            pos == Position::from(3) &&
            incomplete == vec![
                IncompleteAt {
                    address: Address::from_parts(Level::from(0), 0),
                    required_for_witness: true
                },
                IncompleteAt {
                    address: Address::from_parts(Level::from(2), 1),
                    required_for_witness: true
                }
            ]
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(1),
        Err(ShardTreeError::Query(QueryError::TreeIncomplete(v))) if v == vec![Address::from_parts(Level::from(0), 0)]
    );

    assert_matches!(
        tree.batch_insert(
            Position::from(0),
            vec![
                ("a".to_string(), Retention::Ephemeral),
            ].into_iter()
        ),
        Ok(Some((pos, incomplete))) if
            pos == Position::from(0) &&
            incomplete == vec![]
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(0),
        Ok(h) if h == *"abcd____________"
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(1),
        Ok(h) if h == *"ab______________"
    );

    assert_matches!(
        tree.batch_insert(
            Position::from(10),
            vec![
                ("k".to_string(), Retention::Ephemeral),
                ("l".to_string(), Retention::Checkpoint { id: 2, is_marked: false }),
                ("m".to_string(), Retention::Ephemeral),
            ].into_iter()
        ),
        Ok(Some((pos, incomplete))) if
            pos == Position::from(12) &&
            incomplete == vec![
                IncompleteAt {
                    address: Address::from_parts(Level::from(0), 13),
                    required_for_witness: false
                },
                IncompleteAt {
                    address: Address::from_parts(Level::from(1), 7),
                    required_for_witness: false
                },
                IncompleteAt {
                    address: Address::from_parts(Level::from(1), 4),
                    required_for_witness: false
                },
            ]
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(0),
        // The (0, 13) and (1, 7) incomplete subtrees are
        // not considered incomplete here because they appear
        // at the tip of the tree.
        Err(ShardTreeError::Query(QueryError::TreeIncomplete(xs))) if xs == vec![
            Address::from_parts(Level::from(2), 1),
            Address::from_parts(Level::from(1), 4),
        ]
    );

    assert_matches!(tree.truncate_to_depth(1), Ok(true));

    assert_matches!(
        tree.batch_insert(
            Position::from(4),
            ('e'..'k')
                .into_iter()
                .map(|c| (c.to_string(), Retention::Ephemeral))
        ),
        Ok(_)
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(0),
        Ok(h) if h == *"abcdefghijkl____"
    );

    assert_matches!(
        tree.root_at_checkpoint_depth(1),
        Ok(h) if h == *"ab______________"
    );
}

pub fn check_shard_sizes<E: Debug, S: ShardStore<H = String, CheckpointId = u32, Error = E>>(
    mut tree: ShardTree<S, 4, 2>,
) {
    for c in 'a'..'p' {
        tree.append(c.to_string(), Retention::Ephemeral).unwrap();
    }

    assert_eq!(tree.store.get_shard_roots().unwrap().len(), 4);
    assert_eq!(
        tree.store
            .get_shard(Address::from_parts(Level::from(2), 3))
            .unwrap()
            .and_then(|t| t.max_position()),
        Some(Position::from(14))
    );
}

pub fn check_witness_with_pruned_subtrees<
    E: Debug,
    S: ShardStore<H = String, CheckpointId = u32, Error = E>,
>(
    mut tree: ShardTree<S, 6, 3>,
) {
    // introduce some roots
    let shard_root_level = Level::from(3);
    for idx in 0u64..4 {
        let root = if idx == 3 {
            "abcdefgh".to_string()
        } else {
            idx.to_string()
        };
        tree.insert(Address::from_parts(shard_root_level, idx), root)
            .unwrap();
    }

    // simulate discovery of a note
    tree.batch_insert(
        Position::from(24),
        ('a'..='h').into_iter().map(|c| {
            (
                c.to_string(),
                match c {
                    'c' => Retention::Marked,
                    'h' => Retention::Checkpoint {
                        id: 3,
                        is_marked: false,
                    },
                    _ => Retention::Ephemeral,
                },
            )
        }),
    )
    .unwrap();

    // construct a witness for the note
    let witness = tree
        .witness_at_checkpoint_depth(Position::from(26), 0)
        .unwrap();
    assert_eq!(
        witness.path_elems(),
        &[
            "d",
            "ab",
            "efgh",
            "2",
            "01",
            "________________________________"
        ]
    );
}
