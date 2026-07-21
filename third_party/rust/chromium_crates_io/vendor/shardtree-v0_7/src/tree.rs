//! The core tree types.

use std::ops::Deref;
use std::sync::Arc;

use incrementalmerkletree::{Address, Level, Position};

/// A "pattern functor" for a single layer of a binary tree.
///
/// # Type parameters
/// - `C`: the child type (in [`Tree`] this is `Arc<Tree<A, V>>`).
/// - `A`: internal (`Parent`) node data, i.e. the annotation attached to an
///   internal node (for instance the output type of the hash function).
/// - `V`: the value carried on `Leaf` nodes.
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

impl<'a, C, A, V> Node<C, &'a A, &'a V>
where
    C: Clone,
    A: Clone,
    V: Clone,
{
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

/// An immutable binary tree with each of its nodes tagged with an annotation
/// value.
///
/// The tree is homogeneous: every node uses the same `A` and `V` types, so every
/// `Parent` annotation has type `A` and every `Leaf` value has type `V`
/// throughout the whole tree.
///
/// # Type parameters
/// - `A`: internal (`Parent`) node data, i.e. the annotation attached to an
///   internal node (for instance the output type of the hash function).
/// - `V`: the value carried on `Leaf` nodes.
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
    pub fn map<B, F>(&self, f: &F) -> Tree<A, B>
    where
        F: Fn(&V) -> B,
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
    pub fn try_map<B, E, F>(&self, f: &F) -> Result<Tree<A, B>, E>
    where
        F: Fn(&V) -> Result<B, E>,
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
///
/// A `LocatedTree` is a raw [`Tree`] paired with the [`Address`] at which its
/// root sits. The distinction from a bare [`Tree`] is that a `LocatedTree` knows
/// where it lives: it is not a free-floating, context-free structure.
///
/// The address is **absolute**, not relative. The `index` component of an
/// [`Address`] is a global coordinate (the number of subtrees rooted at that
/// level appearing to its left in the whole tree), not an offset relative to
/// some parent tree passed in as context. These absolute coordinates propagate
/// downward: a node's children have indices `index * 2` and `index * 2 + 1`, so
/// every node's address is meaningful in the one canonical global tree.
///
/// What is relative is the inner [`Tree`]: its nodes store no addresses of their
/// own. Their addresses are derived by descending from `root_addr`. So the inner
/// [`Tree`] is context-free, and `root_addr` anchors it to an absolute position.
///
/// # Type parameters
/// - `A`: internal (`Parent`) node data, i.e. the annotation attached to an
///   internal node (for instance the output type of the hash function).
/// - `V`: the value carried on `Leaf` nodes.
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

    /// Returns a multi-line, indented "connector" rendering of this tree for debugging and test
    /// output (top-down, root first), in the style of the `tree` command but using ASCII connectors
    /// `|-- ` and `` `-- ``.
    ///
    /// Each node is shown as its address `[level,index]` followed by its kind:
    ///
    /// * `Parent (ann: <ann>)`, where `<ann>` is produced by `fmt_ann`;
    /// * `Leaf <value>`, where `<value>` is produced by `fmt_val`;
    /// * `Nil` for an empty subtree.
    ///
    /// # Examples
    ///
    /// ```
    /// use incrementalmerkletree::{Address, Level};
    /// use shardtree::{LocatedTree, Tree};
    ///
    /// let tree = LocatedTree::from_parts(
    ///     Address::from_parts(Level::from(2), 0),
    ///     Tree::parent(
    ///         "root",
    ///         Tree::parent("l", Tree::leaf("a"), Tree::leaf("b")),
    ///         Tree::parent("r", Tree::leaf("c"), Tree::leaf("d")),
    ///     ),
    /// )
    /// .unwrap();
    ///
    /// assert_eq!(
    ///     tree.pretty_print_indented_with(
    ///         |ann: &&str| ann.to_string(),
    ///         |value: &&str| value.to_string(),
    ///     ),
    ///     concat!(
    ///         "[2,0] Parent (ann: root)\n",
    ///         "|-- [1,0] Parent (ann: l)\n",
    ///         "|   |-- [0,0] Leaf a\n",
    ///         "|   `-- [0,1] Leaf b\n",
    ///         "`-- [1,1] Parent (ann: r)\n",
    ///         "    |-- [0,2] Leaf c\n",
    ///         "    `-- [0,3] Leaf d",
    ///     ),
    /// );
    /// ```
    pub fn pretty_print_indented_with<FA, FV>(&self, fmt_ann: FA, fmt_val: FV) -> String
    where
        FA: Fn(&A) -> String,
        FV: Fn(&V) -> String,
    {
        // `self_prefix` is printed before this node's label; `child_prefix` is the base prefix that
        // its descendants extend with their own connectors.
        fn go<A, V, FA, FV>(
            out: &mut String,
            addr: Address,
            tree: &Tree<A, V>,
            self_prefix: &str,
            child_prefix: &str,
            fmt_ann: &FA,
            fmt_val: &FV,
        ) where
            FA: Fn(&A) -> String,
            FV: Fn(&V) -> String,
        {
            if !out.is_empty() {
                out.push('\n');
            }
            let loc = format!("[{},{}]", u8::from(addr.level()), addr.index());
            match &tree.0 {
                Node::Parent { ann, left, right } => {
                    out.push_str(&format!(
                        "{self_prefix}{loc} Parent (ann: {})",
                        fmt_ann(ann)
                    ));
                    let (l_addr, r_addr) = addr
                        .children()
                        .expect("a parent node always has child addresses");
                    let l_self = format!("{child_prefix}|-- ");
                    let l_child = format!("{child_prefix}|   ");
                    let r_self = format!("{child_prefix}`-- ");
                    let r_child = format!("{child_prefix}    ");
                    go(out, l_addr, left, &l_self, &l_child, fmt_ann, fmt_val);
                    go(out, r_addr, right, &r_self, &r_child, fmt_ann, fmt_val);
                }
                Node::Leaf { value } => {
                    out.push_str(&format!("{self_prefix}{loc} Leaf {}", fmt_val(value)));
                }
                Node::Nil => {
                    out.push_str(&format!("{self_prefix}{loc} Nil"));
                }
            }
        }

        let mut out = String::new();
        go(
            &mut out,
            self.root_addr,
            &self.root,
            "",
            "",
            &fmt_ann,
            &fmt_val,
        );
        out
    }

    /// Returns a 2D, "inverted tree" rendering of this tree for debugging and test output, drawn
    /// with diagonal `/` and `\` branches that converge downward to the root at the bottom (leaves
    /// and `Nil` nodes are on the top rows).
    ///
    /// Each node is a single token: its address `[level,index]` for parents, `[level,index]:<value>`
    /// for leaves (where `<value>` is produced by `fmt_val`), and `[level,index]:Nil` for empty
    /// subtrees. When `fmt_ann` returns a non-empty string for a parent it is appended as
    /// `[level,index]:<ann>`. Token width drives the spacing, so keep the closure outputs short.
    ///
    /// The rendering assumes ASCII token text.
    ///
    /// # Examples
    ///
    /// ```
    /// use incrementalmerkletree::{Address, Level};
    /// use shardtree::{LocatedTree, Tree};
    ///
    /// let tree = LocatedTree::from_parts(
    ///     Address::from_parts(Level::from(1), 0),
    ///     Tree::parent("root", Tree::leaf("a"), Tree::leaf("b")),
    /// )
    /// .unwrap();
    ///
    /// // Returning an empty annotation keeps parent tokens to their `[level,index]` label.
    /// assert_eq!(
    ///     tree.pretty_print_bottom_top_with(
    ///         |_ann: &&str| String::new(),
    ///         |value: &&str| value.to_string(),
    ///     ),
    ///     concat!(
    ///         "[0,0]:a [0,1]:b\n",
    ///         "    \\     /\n",
    ///         "     \\   /\n",
    ///         "      \\ /\n",
    ///         "     [1,0]",
    ///     ),
    /// );
    /// ```
    pub fn pretty_print_bottom_top_with<FA, FV>(&self, fmt_ann: FA, fmt_val: FV) -> String
    where
        FA: Fn(&A) -> String,
        FV: Fn(&V) -> String,
    {
        // Renders the subtree at `addr` into a block of equal-length lines whose own root token sits
        // on the bottom line, returning the block and the column of that root token's center.
        fn render<A, V, FA, FV>(
            addr: Address,
            tree: &Tree<A, V>,
            fmt_ann: &FA,
            fmt_val: &FV,
        ) -> (Vec<String>, usize)
        where
            FA: Fn(&A) -> String,
            FV: Fn(&V) -> String,
        {
            let level = u8::from(addr.level());
            let index = addr.index();
            match &tree.0 {
                Node::Parent { ann, left, right } => {
                    let (l_addr, r_addr) = addr
                        .children()
                        .expect("a parent node always has child addresses");
                    let (l_block, l_col) = render(l_addr, left, fmt_ann, fmt_val);
                    let (r_block, r_col) = render(r_addr, right, fmt_ann, fmt_val);

                    let l_w = l_block.iter().map(|s| s.len()).max().unwrap_or(0);
                    let r_w = r_block.iter().map(|s| s.len()).max().unwrap_or(0);
                    let h = l_block.len().max(r_block.len());
                    // Bottom-align the two child blocks so both roots land on the same (last) row.
                    let l_pad = h - l_block.len();
                    let r_pad = h - r_block.len();
                    let row_chars =
                        |block: &[String], pad: usize, w: usize, i: usize| -> Vec<char> {
                            if i < pad {
                                vec![' '; w]
                            } else {
                                let mut v: Vec<char> = block[i - pad].chars().collect();
                                v.resize(w, ' ');
                                v
                            }
                        };

                    // Choose the inter-block gap so the two root columns are an even distance apart,
                    // which lets the slope-1 diagonals meet exactly above the centered parent.
                    let lc = l_col;
                    let mut gap = 1usize;
                    let mut rc = l_w + gap + r_col;
                    while (rc - lc) % 2 != 0 {
                        gap += 1;
                        rc = l_w + gap + r_col;
                    }
                    let top_w = l_w + gap + r_w;
                    let hdiag = (rc - lc) / 2 - 1;
                    let parent_col = lc + hdiag + 1;

                    let ann_str = fmt_ann(ann);
                    let token: Vec<char> = if ann_str.is_empty() {
                        format!("[{level},{index}]").chars().collect()
                    } else {
                        format!("[{level},{index}]:{ann_str}").chars().collect()
                    };
                    let tw = token.len();
                    // The centered parent token may extend left of column 0; shift everything right.
                    let token_start = parent_col as isize - (tw as isize) / 2;
                    let left_pad = if token_start < 0 {
                        (-token_start) as usize
                    } else {
                        0
                    };
                    let token_start = (token_start + left_pad as isize) as usize;
                    let width = (top_w + left_pad).max(token_start + tw);
                    let total_h = h + hdiag + 1;

                    let mut grid = vec![vec![' '; width]; total_h];
                    for (i, row) in grid[..h].iter_mut().enumerate() {
                        for (j, ch) in row_chars(&l_block, l_pad, l_w, i).into_iter().enumerate() {
                            if ch != ' ' {
                                row[left_pad + j] = ch;
                            }
                        }
                        for (j, ch) in row_chars(&r_block, r_pad, r_w, i).into_iter().enumerate() {
                            if ch != ' ' {
                                row[left_pad + l_w + gap + j] = ch;
                            }
                        }
                    }
                    for (di, row) in grid[h..h + hdiag].iter_mut().enumerate() {
                        let i = di + 1;
                        row[left_pad + lc + i] = '\\';
                        row[left_pad + rc - i] = '/';
                    }
                    for (j, ch) in token.iter().enumerate() {
                        grid[total_h - 1][token_start + j] = *ch;
                    }

                    let lines = grid
                        .into_iter()
                        .map(|r| r.into_iter().collect::<String>())
                        .collect();
                    (lines, parent_col + left_pad)
                }
                terminal => {
                    let token = match terminal {
                        Node::Leaf { value } => format!("[{level},{index}]:{}", fmt_val(value)),
                        _ => format!("[{level},{index}]:Nil"),
                    };
                    let center = token.chars().count() / 2;
                    (vec![token], center)
                }
            }
        }

        let (lines, _) = render(self.root_addr, &self.root, &fmt_ann, &fmt_val);
        lines
            .into_iter()
            .map(|l| l.trim_end().to_string())
            .collect::<Vec<_>>()
            .join("\n")
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
    pub fn map<B, F>(&self, f: &F) -> LocatedTree<A, B>
    where
        F: Fn(&V) -> B,
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
    pub fn try_map<B, E, F>(&self, f: &F) -> Result<LocatedTree<A, B>, E>
    where
        F: Fn(&V) -> Result<B, E>,
        A: Clone,
    {
        Ok(LocatedTree {
            root_addr: self.root_addr,
            root: self.root.try_map(f)?,
        })
    }
}

impl<A, V> LocatedTree<A, V>
where
    A: Default + Clone,
    V: Clone,
{
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
        fn go<A, V>(
            root_addr: Address,
            root: &Tree<A, V>,
            addr: Address,
        ) -> Option<LocatedTree<A, V>>
        where
            A: Clone,
            V: Clone,
        {
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
        fn go<A, V>(level: Level, root_addr: Address, root: Tree<A, V>) -> Vec<LocatedTree<A, V>>
        where
            A: Clone,
            V: Clone,
        {
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

    pub(crate) fn parent<A, B>(left: Tree<A, B>, right: Tree<A, B>) -> Tree<A, B>
    where
        A: Default,
    {
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
