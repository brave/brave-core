//! The core tree types.

use std::ops::Deref;
use std::sync::Arc;

use incrementalmerkletree::{Address, Level, Position};

/// A "pattern functor" for a single layer of a binary tree.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Node<C, A, V> {
    /// A parent node in the tree, annotated with a value of type `A` and with left and right
    /// children of type `C`.
    Parent { ann: A, left: C, right: C },
    /// A node of the tree that contains a value (usually a hash, sometimes with additional
    /// metadata) and that has no children.
    ///
    /// Note that leaf nodes may appear at any position in the tree; i.e. they may contain computed
    /// subtree root values and not just level-0 leaves.
    Leaf { value: V },
    /// The empty tree; a subtree or leaf for which no information is available.
    Nil,
}

impl<C, A, V> Node<C, A, V> {
    /// Returns whether or not this is the `Nil` tree.
    ///
    /// This is useful for cases where the compiler can automatically dereference an [`Arc`], where
    /// one would otherwise need additional ceremony to make an equality check.
    pub fn is_nil(&self) -> bool {
        matches!(self, Node::Nil)
    }

    /// Returns the contained leaf value, if this is a leaf node.
    pub fn leaf_value(&self) -> Option<&V> {
        match self {
            Node::Parent { .. } => None,
            Node::Leaf { value } => Some(value),
            Node::Nil => None,
        }
    }

    /// Returns the annotation, if this is a parent node.
    pub fn annotation(&self) -> Option<&A> {
        match self {
            Node::Parent { ann, .. } => Some(ann),
            Node::Leaf { .. } => None,
            Node::Nil => None,
        }
    }

    /// Replaces the annotation on this node, if it is a [`Node::Parent`]; otherwise
    /// returns this node unaltered.
    pub fn reannotate(self, ann: A) -> Self {
        match self {
            Node::Parent { left, right, .. } => Node::Parent { ann, left, right },
            other => other,
        }
    }
}

impl<'a, C: Clone, A: Clone, V: Clone> Node<C, &'a A, &'a V> {
    /// Maps a `Node<C, &A, &V>` to a `Node<C, A, V>` by cloning the contents of the node.
    pub fn cloned(&self) -> Node<C, A, V> {
        match self {
            Node::Parent { ann, left, right } => Node::Parent {
                ann: (*ann).clone(),
                left: left.clone(),
                right: right.clone(),
            },
            Node::Leaf { value } => Node::Leaf {
                value: (*value).clone(),
            },
            Node::Nil => Node::Nil,
        }
    }
}

/// An immutable binary tree with each of its nodes tagged with an annotation value.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Tree<A, V>(pub(crate) Node<Arc<Tree<A, V>>, A, V>);

impl<A, V> Deref for Tree<A, V> {
    type Target = Node<Arc<Tree<A, V>>, A, V>;
    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl<A, V> Tree<A, V> {
    /// Constructs the empty tree.
    ///
    /// This represents a tree for which we have no information.
    pub const fn empty() -> Self {
        Tree(Node::Nil)
    }

    /// Constructs a tree containing a single leaf.
    ///
    /// This represents either leaf of the tree, or an internal parent node of the
    /// tree whose children have all been pruned.
    pub fn leaf(value: V) -> Self {
        Tree(Node::Leaf { value })
    }

    /// Constructs a tree containing a pair of leaves.
    pub fn parent(ann: A, left: Self, right: Self) -> Self {
        Tree(Node::Parent {
            ann,
            left: Arc::new(left),
            right: Arc::new(right),
        })
    }

    /// Returns `true` if the tree is the [`Node::Nil`] node.
    pub fn is_empty(&self) -> bool {
        self.0.is_nil()
    }

    /// Replaces the annotation at the root of the tree, if the root is a [`Node::Parent`];
    /// otherwise returns this tree unaltered.
    pub fn reannotate_root(self, ann: A) -> Self {
        Tree(self.0.reannotate(ann))
    }

    /// Returns `true` this is a [`Node::Leaf`], `false` otherwise.
    pub fn is_leaf(&self) -> bool {
        matches!(&self.0, Node::Leaf { .. })
    }

    /// Returns a vector of the addresses of [`Node::Nil`] subtree roots
    /// within this tree.
    ///
    /// The given address must correspond to the root of this tree, or this method will
    /// yield incorrect results or may panic.
    pub fn incomplete_nodes(&self, root_addr: Address) -> Vec<Address> {
        match &self.0 {
            Node::Parent { left, right, .. } => {
                // We should never construct parent nodes where both children are Nil.
                // While we could handle that here, if we encountered that case it would
                // be indicative of a programming error elsewhere and so we assert instead.
                assert!(!(left.0.is_nil() && right.0.is_nil()));
                let (left_root, right_root) = root_addr
                    .children()
                    .expect("A parent node cannot appear at level 0");

                let mut left_incomplete = left.incomplete_nodes(left_root);
                let mut right_incomplete = right.incomplete_nodes(right_root);
                left_incomplete.append(&mut right_incomplete);
                left_incomplete
            }
            Node::Leaf { .. } => vec![],
            Node::Nil => vec![root_addr],
        }
    }

    /// Applies the provided function to each leaf of the tree and returns
    /// a new tree having the same structure as the original.
    pub fn map<B, F: Fn(&V) -> B>(&self, f: &F) -> Tree<A, B>
    where
        A: Clone,
    {
        Tree(match &self.0 {
            Node::Parent { ann, left, right } => Node::Parent {
                ann: ann.clone(),
                left: Arc::new(left.map(f)),
                right: Arc::new(right.map(f)),
            },
            Node::Leaf { value } => Node::Leaf { value: f(value) },
            Node::Nil => Node::Nil,
        })
    }

    /// Applies the provided function to each leaf of the tree and returns
    /// a new tree having the same structure as the original, or an error
    /// if any transformation of the leaf fails.
    pub fn try_map<B, E, F: Fn(&V) -> Result<B, E>>(&self, f: &F) -> Result<Tree<A, B>, E>
    where
        A: Clone,
    {
        Ok(Tree(match &self.0 {
            Node::Parent { ann, left, right } => Node::Parent {
                ann: ann.clone(),
                left: Arc::new(left.try_map(f)?),
                right: Arc::new(right.try_map(f)?),
            },
            Node::Leaf { value } => Node::Leaf { value: f(value)? },
            Node::Nil => Node::Nil,
        }))
    }
}

/// A binary Merkle tree with its root at the given address.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct LocatedTree<A, V> {
    pub(crate) root_addr: Address,
    pub(crate) root: Tree<A, V>,
}

impl<A, V> LocatedTree<A, V> {
    /// Constructs a new LocatedTree from its constituent parts.
    ///
    /// Returns the newly constructed LocatedTree, or the address at which the provided tree extends
    /// beyond the position range of the provided root address.
    pub fn from_parts(root_addr: Address, root: Tree<A, V>) -> Result<Self, Address> {
        // In order to meet various pre-conditions throughout the crate, we require that
        // no `Node::Parent` in `root` has a level of 0 relative to `root_addr`.
        fn check<A, V>(addr: Address, root: &Tree<A, V>) -> Result<(), Address> {
            match (&root.0, addr.children()) {
                // Found an inconsistency!
                (Node::Parent { .. }, None) => Err(addr),
                // Check consistency of children recursively.
                (Node::Parent { left, right, .. }, Some((l_addr, r_addr))) => {
                    check(l_addr, left)?;
                    check(r_addr, right)?;
                    Ok(())
                }

                // Leaves are technically allowed to occur at any level, so we do not
                // require `addr` to have no children.
                (Node::Leaf { .. }, _) => Ok(()),

                // Nil nodes have no information, so we cannot verify that the data it
                // represents is consistent with `root_addr`. Instead we rely on methods
                // that mutate `LocatedTree` to verify that the insertion address is not
                // inconsistent with `root_addr`.
                (Node::Nil, _) => Ok(()),
            }
        }

        check(root_addr, &root).map(|_| LocatedTree { root_addr, root })
    }

    /// Returns the root address of this tree.
    pub fn root_addr(&self) -> Address {
        self.root_addr
    }

    /// Returns a reference to the root of the tree.
    pub fn root(&self) -> &Tree<A, V> {
        &self.root
    }

    /// Consumes this tree and returns its root as an owned value.
    pub fn take_root(self) -> Tree<A, V> {
        self.root
    }

    /// Returns a new [`LocatedTree`] with the provided value replacing the annotation of its root
    /// node, if that root node is a [`Node::Parent`]. Otherwise returns this tree unaltered.
    pub fn reannotate_root(self, value: A) -> Self {
        LocatedTree {
            root_addr: self.root_addr,
            root: self.root.reannotate_root(value),
        }
    }

    /// Returns the set of incomplete subtree roots contained within this tree, ordered by
    /// increasing position.
    pub fn incomplete_nodes(&self) -> Vec<Address> {
        self.root.incomplete_nodes(self.root_addr)
    }

    /// Returns the value at the specified position, if any.
    pub fn value_at_position(&self, position: Position) -> Option<&V> {
        /// Pre-condition: `addr` must be the address of `root`.
        fn go<A, V>(pos: Position, addr: Address, root: &Tree<A, V>) -> Option<&V> {
            match &root.0 {
                Node::Parent { left, right, .. } => {
                    let (l_addr, r_addr) = addr
                        .children()
                        .expect("has children because we checked `root` is a parent");
                    if l_addr.position_range().contains(&pos) {
                        go(pos, l_addr, left)
                    } else {
                        go(pos, r_addr, right)
                    }
                }
                Node::Leaf { value } if addr.level() == Level::from(0) => Some(value),
                _ => None,
            }
        }

        if self.root_addr.position_range().contains(&position) {
            go(position, self.root_addr, &self.root)
        } else {
            None
        }
    }

    /// Applies the provided function to each leaf of the tree and returns
    /// a new tree having the same structure as the original.
    pub fn map<B, F: Fn(&V) -> B>(&self, f: &F) -> LocatedTree<A, B>
    where
        A: Clone,
    {
        LocatedTree {
            root_addr: self.root_addr,
            root: self.root.map(f),
        }
    }

    /// Applies the provided function to each leaf of the tree and returns
    /// a new tree having the same structure as the original, or an error
    /// if any transformation of the leaf fails.
    pub fn try_map<B, E, F: Fn(&V) -> Result<B, E>>(&self, f: &F) -> Result<LocatedTree<A, B>, E>
    where
        A: Clone,
    {
        Ok(LocatedTree {
            root_addr: self.root_addr,
            root: self.root.try_map(f)?,
        })
    }
}

impl<A: Default + Clone, V: Clone> LocatedTree<A, V> {
    /// Constructs a new empty tree with its root at the provided address.
    pub fn empty(root_addr: Address) -> Self {
        Self {
            root_addr,
            root: Tree::empty(),
        }
    }

    /// Constructs a new tree consisting of a single leaf with the provided value, and the
    /// specified root address.
    pub fn with_root_value(root_addr: Address, value: V) -> Self {
        Self {
            root_addr,
            root: Tree::leaf(value),
        }
    }

    /// Traverses this tree to find the child node at the specified address and returns it.
    ///
    /// Returns `None` if the specified address is not a descendant of this tree's root address, or
    /// if the tree is terminated by a [`Node::Nil`] or leaf node before the specified address can
    /// be reached.
    pub fn subtree(&self, addr: Address) -> Option<Self> {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<A: Clone, V: Clone>(
            root_addr: Address,
            root: &Tree<A, V>,
            addr: Address,
        ) -> Option<LocatedTree<A, V>> {
            if root_addr == addr {
                Some(LocatedTree {
                    root_addr,
                    root: root.clone(),
                })
            } else {
                match &root.0 {
                    Node::Parent { left, right, .. } => {
                        let (l_addr, r_addr) = root_addr
                            .children()
                            .expect("has children because we checked `root` is a parent");
                        if l_addr.contains(&addr) {
                            go(l_addr, left.as_ref(), addr)
                        } else {
                            go(r_addr, right.as_ref(), addr)
                        }
                    }
                    _ => None,
                }
            }
        }

        if self.root_addr.contains(&addr) {
            go(self.root_addr, &self.root, addr)
        } else {
            None
        }
    }

    /// Decomposes this tree into the vector of its subtrees having height `level + 1`.
    ///
    /// If this root address of this tree is lower down in the tree than the level specified,
    /// the entire tree is returned as the sole element of the result vector.
    pub fn decompose_to_level(self, level: Level) -> Vec<Self> {
        /// Pre-condition: `root_addr` must be the address of `root`.
        fn go<A: Clone, V: Clone>(
            level: Level,
            root_addr: Address,
            root: Tree<A, V>,
        ) -> Vec<LocatedTree<A, V>> {
            if root_addr.level() == level {
                vec![LocatedTree { root_addr, root }]
            } else {
                match root.0 {
                    Node::Parent { left, right, .. } => {
                        let (l_addr, r_addr) = root_addr
                            .children()
                            .expect("has children because we checked `root` is a parent");
                        let mut l_decomposed = go(
                            level,
                            l_addr,
                            Arc::try_unwrap(left).unwrap_or_else(|rc| (*rc).clone()),
                        );
                        let mut r_decomposed = go(
                            level,
                            r_addr,
                            Arc::try_unwrap(right).unwrap_or_else(|rc| (*rc).clone()),
                        );
                        l_decomposed.append(&mut r_decomposed);
                        l_decomposed
                    }
                    _ => vec![],
                }
            }
        }

        if level >= self.root_addr.level() {
            vec![self]
        } else {
            go(level, self.root_addr, self.root)
        }
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use incrementalmerkletree::{Address, Level};

    use super::{LocatedTree, Tree};

    pub(crate) fn str_leaf<A>(c: &str) -> Tree<A, String> {
        Tree::leaf(c.to_string())
    }

    pub(crate) fn nil<A, B>() -> Tree<A, B> {
        Tree::empty()
    }

    pub(crate) fn leaf<A, B>(value: B) -> Tree<A, B> {
        Tree::leaf(value)
    }

    pub(crate) fn parent<A: Default, B>(left: Tree<A, B>, right: Tree<A, B>) -> Tree<A, B> {
        Tree::parent(A::default(), left, right)
    }

    #[test]
    fn incomplete_nodes() {
        let t: Tree<(), String> = parent(nil(), str_leaf("a"));
        assert_eq!(
            t.incomplete_nodes(Address::from_parts(Level::from(1), 0)),
            vec![Address::from_parts(Level::from(0), 0)]
        );

        let t0 = parent(str_leaf("b"), t.clone());
        assert_eq!(
            t0.incomplete_nodes(Address::from_parts(Level::from(2), 1)),
            vec![Address::from_parts(Level::from(0), 6)]
        );

        let t1 = parent(nil(), t);
        assert_eq!(
            t1.incomplete_nodes(Address::from_parts(Level::from(2), 1)),
            vec![
                Address::from_parts(Level::from(1), 2),
                Address::from_parts(Level::from(0), 6)
            ]
        );
    }

    #[test]
    fn located() {
        let l = parent(str_leaf("a"), str_leaf("b"));
        let r = parent(str_leaf("c"), str_leaf("d"));

        let t: LocatedTree<(), String> = LocatedTree {
            root_addr: Address::from_parts(2.into(), 1),
            root: parent(l.clone(), r.clone()),
        };

        assert_eq!(t.value_at_position(5.into()), Some(&"b".to_string()));
        assert_eq!(t.value_at_position(8.into()), None);
        assert_eq!(t.subtree(Address::from_parts(0.into(), 1)), None);
        assert_eq!(t.subtree(Address::from_parts(3.into(), 0)), None);

        let subtree_addr = Address::from_parts(1.into(), 3);
        assert_eq!(
            t.subtree(subtree_addr),
            Some(LocatedTree {
                root_addr: subtree_addr,
                root: r.clone()
            })
        );

        assert_eq!(
            t.decompose_to_level(1.into()),
            vec![
                LocatedTree {
                    root_addr: Address::from_parts(1.into(), 2),
                    root: l,
                },
                LocatedTree {
                    root_addr: Address::from_parts(1.into(), 3),
                    root: r,
                }
            ]
        );
    }
}
