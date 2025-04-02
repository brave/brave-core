//! Vec-backed ID-tree.
//!
//! # Behavior
//!
//! - Trees have at least a root node;
//! - Nodes have zero or more ordered children;
//! - Nodes have at most one parent;
//! - Nodes can be detached (orphaned) but not removed;
//! - Node parent, next sibling, previous sibling, first child and last child
//!   can be accessed in constant time;
//! - All methods perform in constant time;
//! - All iterators perform in linear time.
//!
//! # Examples
//!
//! ```
//! let mut tree = ego_tree::Tree::new('a');
//! let mut root = tree.root_mut();
//! root.append('b');
//! let mut c = root.append('c');
//! c.append('d');
//! c.append('e');
//! ```
//!
//! ```
//! #[macro_use] extern crate ego_tree;
//! # fn main() {
//! let tree = tree!('a' => { 'b', 'c' => { 'd', 'e' } });
//! # }
//! ```

#![warn(
    missing_docs,
    missing_debug_implementations,
    missing_copy_implementations
)]

use std::fmt::{self, Debug, Display, Formatter};
use std::num::NonZeroUsize;

#[cfg(feature = "serde")]
pub mod serde;

/// Vec-backed ID-tree.
///
/// Always contains at least a root node.
#[derive(Clone, PartialEq, Eq, Hash)]
pub struct Tree<T> {
    vec: Vec<Node<T>>,
}

/// Node ID.
///
/// Index into a `Tree`-internal `Vec`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct NodeId(NonZeroUsize);

impl NodeId {
    // Safety: `n` must not equal `usize::MAX`.
    // (This is never the case for `Vec::len()`, that would mean it owns
    // the entire address space without leaving space even for its pointer.)
    unsafe fn from_index(n: usize) -> Self {
        NodeId(NonZeroUsize::new_unchecked(n + 1))
    }

    fn to_index(self) -> usize {
        self.0.get() - 1
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct Node<T> {
    parent: Option<NodeId>,
    prev_sibling: Option<NodeId>,
    next_sibling: Option<NodeId>,
    children: Option<(NodeId, NodeId)>,
    value: T,
}

fn _static_assert_size_of_node() {
    // "Instantiating" the generic `transmute` function without calling it
    // still triggers the magic compile-time check
    // that input and output types have the same `size_of()`.
    let _ = std::mem::transmute::<Node<()>, [usize; 5]>;
}

impl<T> Node<T> {
    fn new(value: T) -> Self {
        Node {
            parent: None,
            prev_sibling: None,
            next_sibling: None,
            children: None,
            value,
        }
    }

    pub fn map<F, U>(self, mut transform: F) -> Node<U>
    where
        F: FnMut(T) -> U,
    {
        Node {
            parent: self.parent,
            prev_sibling: self.prev_sibling,
            next_sibling: self.next_sibling,
            children: self.children,
            value: transform(self.value),
        }
    }

    pub fn map_ref<F, U>(&self, mut transform: F) -> Node<U>
    where
        F: FnMut(&T) -> U,
    {
        Node {
            parent: self.parent,
            prev_sibling: self.prev_sibling,
            next_sibling: self.next_sibling,
            children: self.children,
            value: transform(&self.value),
        }
    }
}

/// Node reference.
#[derive(Debug)]
pub struct NodeRef<'a, T: 'a> {
    /// Node ID.
    id: NodeId,

    /// Tree containing the node.
    tree: &'a Tree<T>,

    node: &'a Node<T>,
}

/// Node mutator.
#[derive(Debug)]
pub struct NodeMut<'a, T: 'a> {
    /// Node ID.
    id: NodeId,

    /// Tree containing the node.
    tree: &'a mut Tree<T>,
}

// Trait implementations regardless of T.

impl<'a, T: 'a> Copy for NodeRef<'a, T> {}
impl<'a, T: 'a> Clone for NodeRef<'a, T> {
    fn clone(&self) -> Self {
        *self
    }
}

impl<'a, T: 'a> Eq for NodeRef<'a, T> {}
impl<'a, T: 'a> PartialEq for NodeRef<'a, T> {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
            && std::ptr::eq(self.tree, other.tree)
            && std::ptr::eq(self.node, other.node)
    }
}

impl<T> Tree<T> {
    /// Creates a tree with a root node.
    pub fn new(root: T) -> Self {
        Tree {
            vec: vec![Node::new(root)],
        }
    }

    /// Creates a tree with a root node and the specified capacity.
    pub fn with_capacity(root: T, capacity: usize) -> Self {
        let mut vec = Vec::with_capacity(capacity);
        vec.push(Node::new(root));
        Tree { vec }
    }

    /// Returns a reference to the specified node.
    pub fn get(&self, id: NodeId) -> Option<NodeRef<T>> {
        self.vec.get(id.to_index()).map(|node| NodeRef {
            id,
            node,
            tree: self,
        })
    }

    /// Returns a mutator of the specified node.
    pub fn get_mut(&mut self, id: NodeId) -> Option<NodeMut<T>> {
        let exists = self.vec.get(id.to_index()).map(|_| ());
        exists.map(move |_| NodeMut { id, tree: self })
    }

    unsafe fn node(&self, id: NodeId) -> &Node<T> {
        self.vec.get_unchecked(id.to_index())
    }

    unsafe fn node_mut(&mut self, id: NodeId) -> &mut Node<T> {
        self.vec.get_unchecked_mut(id.to_index())
    }

    /// Returns a reference to the specified node.
    /// # Safety
    /// The caller must ensure that `id` is a valid node ID.
    pub unsafe fn get_unchecked(&self, id: NodeId) -> NodeRef<T> {
        NodeRef {
            id,
            node: self.node(id),
            tree: self,
        }
    }

    /// Returns a mutator of the specified node.
    /// # Safety
    /// The caller must ensure that `id` is a valid node ID.
    pub unsafe fn get_unchecked_mut(&mut self, id: NodeId) -> NodeMut<T> {
        NodeMut { id, tree: self }
    }

    /// Returns a reference to the root node.
    pub fn root(&self) -> NodeRef<T> {
        unsafe { self.get_unchecked(NodeId::from_index(0)) }
    }

    /// Returns a mutator of the root node.
    pub fn root_mut(&mut self) -> NodeMut<T> {
        unsafe { self.get_unchecked_mut(NodeId::from_index(0)) }
    }

    /// Creates an orphan node.
    pub fn orphan(&mut self, value: T) -> NodeMut<T> {
        let id = unsafe { NodeId::from_index(self.vec.len()) };
        self.vec.push(Node::new(value));
        unsafe { self.get_unchecked_mut(id) }
    }

    /// Merge with another tree as orphan, returning the new root of tree being merged.
    // Allowing this for compactness.
    #[allow(clippy::option_map_unit_fn)]
    pub fn extend_tree(&mut self, mut other_tree: Tree<T>) -> NodeMut<T> {
        let offset = self.vec.len();
        let offset_id = |id: NodeId| -> NodeId {
            let old_index = id.to_index();
            let new_index = old_index + offset;
            unsafe { NodeId::from_index(new_index) }
        };
        let other_tree_root_id = offset_id(other_tree.root().id);
        for node in other_tree.vec.iter_mut() {
            node.parent.as_mut().map(|id| *id = offset_id(*id));
            node.prev_sibling.as_mut().map(|id| *id = offset_id(*id));
            node.next_sibling.as_mut().map(|id| *id = offset_id(*id));
            node.children.as_mut().map(|(id1, id2)| {
                *id1 = offset_id(*id1);
                *id2 = offset_id(*id2);
            });
        }
        self.vec.extend(other_tree.vec);
        unsafe { self.get_unchecked_mut(other_tree_root_id) }
    }

    /// Maps a `Tree<T>` to `Tree<U>` by applying a function to all node values,
    /// copying over the tree's structure and node ids untouched, consuming `self`.
    pub fn map<F, U>(self, mut transform: F) -> Tree<U>
    where
        F: FnMut(T) -> U,
    {
        Tree {
            vec: self
                .vec
                .into_iter()
                .map(|node| node.map(&mut transform))
                .collect(),
        }
    }

    /// Maps a `&Tree<T>` to `Tree<U>` by applying a function to all node values,
    /// copying over the tree's structure and node ids untouched.
    pub fn map_ref<F, U>(&self, mut transform: F) -> Tree<U>
    where
        F: FnMut(&T) -> U,
    {
        Tree {
            vec: self
                .vec
                .iter()
                .map(|node| node.map_ref(&mut transform))
                .collect(),
        }
    }
}

impl<'a, T: 'a> NodeRef<'a, T> {
    /// Returns the ID of this node.
    pub fn id(&self) -> NodeId {
        self.id
    }

    /// Returns the tree owning this node.
    pub fn tree(&self) -> &'a Tree<T> {
        self.tree
    }

    /// Returns the value of this node.
    pub fn value(&self) -> &'a T {
        &self.node.value
    }

    fn axis<F>(&self, f: F) -> Option<Self>
    where
        F: FnOnce(&Node<T>) -> Option<NodeId>,
    {
        f(self.node).map(|id| unsafe { self.tree.get_unchecked(id) })
    }

    /// Returns the parent of this node.
    pub fn parent(&self) -> Option<Self> {
        self.axis(|node| node.parent)
    }

    /// Returns the previous sibling of this node.
    pub fn prev_sibling(&self) -> Option<Self> {
        self.axis(|node| node.prev_sibling)
    }

    /// Returns the next sibling of this node.
    pub fn next_sibling(&self) -> Option<Self> {
        self.axis(|node| node.next_sibling)
    }

    /// Returns the first child of this node.
    pub fn first_child(&self) -> Option<Self> {
        self.axis(|node| node.children.map(|(id, _)| id))
    }

    /// Returns the last child of this node.
    pub fn last_child(&self) -> Option<Self> {
        self.axis(|node| node.children.map(|(_, id)| id))
    }

    /// Returns true if this node has siblings.
    pub fn has_siblings(&self) -> bool {
        self.node.prev_sibling.is_some() || self.node.next_sibling.is_some()
    }

    /// Returns true if this node has children.
    pub fn has_children(&self) -> bool {
        self.node.children.is_some()
    }
}

impl<'a, T: 'a> NodeMut<'a, T> {
    /// Returns the ID of this node.
    pub fn id(&self) -> NodeId {
        self.id
    }

    /// Returns the tree owning this node.
    pub fn tree(&mut self) -> &mut Tree<T> {
        self.tree
    }

    fn node(&mut self) -> &mut Node<T> {
        unsafe { self.tree.node_mut(self.id) }
    }

    /// Returns the value of this node.
    pub fn value(&mut self) -> &mut T {
        &mut self.node().value
    }

    fn axis<F>(&mut self, f: F) -> Option<NodeMut<T>>
    where
        F: FnOnce(&mut Node<T>) -> Option<NodeId>,
    {
        let id = f(self.node());
        id.map(move |id| unsafe { self.tree.get_unchecked_mut(id) })
    }

    fn into_axis<F>(mut self, f: F) -> Result<Self, Self>
    where
        F: FnOnce(&mut Node<T>) -> Option<NodeId>,
    {
        let id = f(self.node());
        match id {
            Some(id) => Ok(unsafe { self.tree.get_unchecked_mut(id) }),
            None => Err(self),
        }
    }

    /// Returns the parent of this node.
    pub fn parent(&mut self) -> Option<NodeMut<T>> {
        self.axis(|node| node.parent)
    }

    /// Returns the parent of this node.
    ///
    /// Returns `Ok(parent)` if possible and `Err(self)` otherwise
    /// so the caller can recover the current position.
    pub fn into_parent(self) -> Result<Self, Self> {
        self.into_axis(|node| node.parent)
    }

    /// Returns the previous sibling of this node.
    pub fn prev_sibling(&mut self) -> Option<NodeMut<T>> {
        self.axis(|node| node.prev_sibling)
    }

    /// Returns the previous sibling of this node.
    ///
    /// Returns `Ok(prev_sibling)` if possible and `Err(self)` otherwise
    /// so the caller can recover the current position.
    pub fn into_prev_sibling(self) -> Result<Self, Self> {
        self.into_axis(|node| node.prev_sibling)
    }

    /// Returns the next sibling of this node.
    pub fn next_sibling(&mut self) -> Option<NodeMut<T>> {
        self.axis(|node| node.next_sibling)
    }

    /// Returns the next sibling of this node.
    ///
    /// Returns `Ok(next_sibling)` if possible and `Err(self)` otherwise
    /// so the caller can recover the current position.
    pub fn into_next_sibling(self) -> Result<Self, Self> {
        self.into_axis(|node| node.next_sibling)
    }

    /// Returns the first child of this node.
    pub fn first_child(&mut self) -> Option<NodeMut<T>> {
        self.axis(|node| node.children.map(|(id, _)| id))
    }

    /// Returns the first child of this node.
    ///
    /// Returns `Ok(first_child)` if possible and `Err(self)` otherwise
    /// so the caller can recover the current position.
    pub fn into_first_child(self) -> Result<Self, Self> {
        self.into_axis(|node| node.children.map(|(id, _)| id))
    }

    /// Returns the last child of this node.
    pub fn last_child(&mut self) -> Option<NodeMut<T>> {
        self.axis(|node| node.children.map(|(_, id)| id))
    }

    /// Returns the last child of this node.
    ///
    /// Returns `Ok(last_child)` if possible and `Err(self)` otherwise
    /// so the caller can recover the current position.
    pub fn into_last_child(self) -> Result<Self, Self> {
        self.into_axis(|node| node.children.map(|(_, id)| id))
    }

    /// Returns true if this node has siblings.
    pub fn has_siblings(&self) -> bool {
        unsafe { self.tree.get_unchecked(self.id).has_siblings() }
    }

    /// Returns true if this node has children.
    pub fn has_children(&self) -> bool {
        unsafe { self.tree.get_unchecked(self.id).has_children() }
    }

    /// Appends a new child to this node.
    pub fn append(&mut self, value: T) -> NodeMut<T> {
        let id = self.tree.orphan(value).id;
        self.append_id(id)
    }

    /// Prepends a new child to this node.
    pub fn prepend(&mut self, value: T) -> NodeMut<T> {
        let id = self.tree.orphan(value).id;
        self.prepend_id(id)
    }

    /// Appends a subtree, return the root of the merged subtree.
    pub fn append_subtree(&mut self, subtree: Tree<T>) -> NodeMut<T> {
        let root_id = self.tree.extend_tree(subtree).id;
        self.append_id(root_id)
    }

    /// Prepends a subtree, return the root of the merged subtree.
    pub fn prepend_subtree(&mut self, subtree: Tree<T>) -> NodeMut<T> {
        let root_id = self.tree.extend_tree(subtree).id;
        self.prepend_id(root_id)
    }

    /// Inserts a new sibling before this node.
    ///
    /// # Panics
    ///
    /// Panics if this node is an orphan.
    pub fn insert_before(&mut self, value: T) -> NodeMut<T> {
        let id = self.tree.orphan(value).id;
        self.insert_id_before(id)
    }

    /// Inserts a new sibling after this node.
    ///
    /// # Panics
    ///
    /// Panics if this node is an orphan.
    pub fn insert_after(&mut self, value: T) -> NodeMut<T> {
        let id = self.tree.orphan(value).id;
        self.insert_id_after(id)
    }

    /// Detaches this node from its parent.
    pub fn detach(&mut self) {
        let parent_id = match self.node().parent {
            Some(id) => id,
            None => return,
        };
        let prev_sibling_id = self.node().prev_sibling;
        let next_sibling_id = self.node().next_sibling;

        {
            self.node().parent = None;
            self.node().prev_sibling = None;
            self.node().next_sibling = None;
        }

        if let Some(id) = prev_sibling_id {
            unsafe {
                self.tree.node_mut(id).next_sibling = next_sibling_id;
            }
        }
        if let Some(id) = next_sibling_id {
            unsafe {
                self.tree.node_mut(id).prev_sibling = prev_sibling_id;
            }
        }

        let parent = unsafe { self.tree.node_mut(parent_id) };
        let (first_child_id, last_child_id) = parent.children.unwrap();
        if first_child_id == last_child_id {
            parent.children = None;
        } else if first_child_id == self.id {
            parent.children = Some((next_sibling_id.unwrap(), last_child_id));
        } else if last_child_id == self.id {
            parent.children = Some((first_child_id, prev_sibling_id.unwrap()));
        }
    }

    /// Appends a child to this node.
    ///
    /// # Panics
    ///
    /// Panics if `new_child_id` is not valid.
    pub fn append_id(&mut self, new_child_id: NodeId) -> NodeMut<T> {
        assert_ne!(
            self.id(),
            new_child_id,
            "Cannot append node as a child to itself"
        );

        let last_child_id = self.node().children.map(|(_, id)| id);

        if last_child_id != Some(new_child_id) {
            {
                let mut new_child = self.tree.get_mut(new_child_id).unwrap();
                new_child.detach();
                new_child.node().parent = Some(self.id);
                new_child.node().prev_sibling = last_child_id;
            }

            if let Some(id) = last_child_id {
                unsafe {
                    self.tree.node_mut(id).next_sibling = Some(new_child_id);
                }
            }

            {
                self.node().children = match self.node().children {
                    Some((first_child_id, _)) => Some((first_child_id, new_child_id)),
                    None => Some((new_child_id, new_child_id)),
                };
            }
        }

        unsafe { self.tree.get_unchecked_mut(new_child_id) }
    }

    /// Prepends a child to this node.
    ///
    /// # Panics
    ///
    /// Panics if `new_child_id` is not valid.
    pub fn prepend_id(&mut self, new_child_id: NodeId) -> NodeMut<T> {
        assert_ne!(
            self.id(),
            new_child_id,
            "Cannot prepend node as a child to itself"
        );

        let first_child_id = self.node().children.map(|(id, _)| id);

        if first_child_id != Some(new_child_id) {
            {
                let mut new_child = self.tree.get_mut(new_child_id).unwrap();
                new_child.detach();
                new_child.node().parent = Some(self.id);
                new_child.node().next_sibling = first_child_id;
            }

            if let Some(id) = first_child_id {
                unsafe {
                    self.tree.node_mut(id).prev_sibling = Some(new_child_id);
                }
            }

            {
                self.node().children = match self.node().children {
                    Some((_, last_child_id)) => Some((new_child_id, last_child_id)),
                    None => Some((new_child_id, new_child_id)),
                };
            }
        }

        unsafe { self.tree.get_unchecked_mut(new_child_id) }
    }

    /// Inserts a sibling before this node.
    ///
    /// # Panics
    ///
    /// - Panics if `new_sibling_id` is not valid.
    /// - Panics if this node is an orphan.
    pub fn insert_id_before(&mut self, new_sibling_id: NodeId) -> NodeMut<T> {
        assert_ne!(
            self.id(),
            new_sibling_id,
            "Cannot insert node as a sibling of itself"
        );

        let parent_id = self.node().parent.unwrap();
        let prev_sibling_id = self.node().prev_sibling;

        {
            let mut new_sibling = self.tree.get_mut(new_sibling_id).unwrap();
            new_sibling.detach();
            new_sibling.node().parent = Some(parent_id);
            new_sibling.node().prev_sibling = prev_sibling_id;
            new_sibling.node().next_sibling = Some(self.id);
        }

        if let Some(id) = prev_sibling_id {
            unsafe {
                self.tree.node_mut(id).next_sibling = Some(new_sibling_id);
            }
        }

        self.node().prev_sibling = Some(new_sibling_id);

        {
            let parent = unsafe { self.tree.node_mut(parent_id) };
            let (first_child_id, last_child_id) = parent.children.unwrap();
            if first_child_id == self.id {
                parent.children = Some((new_sibling_id, last_child_id));
            }
        }

        unsafe { self.tree.get_unchecked_mut(new_sibling_id) }
    }

    /// Inserts a sibling after this node.
    ///
    /// # Panics
    ///
    /// - Panics if `new_sibling_id` is not valid.
    /// - Panics if this node is an orphan.
    pub fn insert_id_after(&mut self, new_sibling_id: NodeId) -> NodeMut<T> {
        assert_ne!(
            self.id(),
            new_sibling_id,
            "Cannot insert node as a sibling of itself"
        );

        let parent_id = self.node().parent.unwrap();
        let next_sibling_id = self.node().next_sibling;

        {
            let mut new_sibling = self.tree.get_mut(new_sibling_id).unwrap();
            new_sibling.detach();
            new_sibling.node().parent = Some(parent_id);
            new_sibling.node().prev_sibling = Some(self.id);
            new_sibling.node().next_sibling = next_sibling_id;
        }

        if let Some(id) = next_sibling_id {
            unsafe {
                self.tree.node_mut(id).prev_sibling = Some(new_sibling_id);
            }
        }

        self.node().next_sibling = Some(new_sibling_id);

        {
            let parent = unsafe { self.tree.node_mut(parent_id) };
            let (first_child_id, last_child_id) = parent.children.unwrap();
            if last_child_id == self.id {
                parent.children = Some((first_child_id, new_sibling_id));
            }
        }

        unsafe { self.tree.get_unchecked_mut(new_sibling_id) }
    }

    /// Reparents the children of a node, appending them to this node.
    ///
    /// # Panics
    ///
    /// Panics if `from_id` is not valid.
    pub fn reparent_from_id_append(&mut self, from_id: NodeId) {
        assert_ne!(
            self.id(),
            from_id,
            "Cannot reparent node's children to itself"
        );

        let new_child_ids = {
            let mut from = self.tree.get_mut(from_id).unwrap();
            match from.node().children.take() {
                Some(ids) => ids,
                None => return,
            }
        };

        unsafe {
            self.tree.node_mut(new_child_ids.0).parent = Some(self.id);
            self.tree.node_mut(new_child_ids.1).parent = Some(self.id);
        }

        if self.node().children.is_none() {
            self.node().children = Some(new_child_ids);
            return;
        }

        let old_child_ids = self.node().children.unwrap();
        unsafe {
            self.tree.node_mut(old_child_ids.1).next_sibling = Some(new_child_ids.0);
            self.tree.node_mut(new_child_ids.0).prev_sibling = Some(old_child_ids.1);
        }

        self.node().children = Some((old_child_ids.0, new_child_ids.1));
    }

    /// Reparents the children of a node, prepending them to this node.
    ///
    /// # Panics
    ///
    /// Panics if `from_id` is not valid.
    pub fn reparent_from_id_prepend(&mut self, from_id: NodeId) {
        assert_ne!(
            self.id(),
            from_id,
            "Cannot reparent node's children to itself"
        );

        let new_child_ids = {
            let mut from = self.tree.get_mut(from_id).unwrap();
            match from.node().children.take() {
                Some(ids) => ids,
                None => return,
            }
        };

        unsafe {
            self.tree.node_mut(new_child_ids.0).parent = Some(self.id);
            self.tree.node_mut(new_child_ids.1).parent = Some(self.id);
        }

        if self.node().children.is_none() {
            self.node().children = Some(new_child_ids);
            return;
        }

        let old_child_ids = self.node().children.unwrap();
        unsafe {
            self.tree.node_mut(old_child_ids.0).prev_sibling = Some(new_child_ids.1);
            self.tree.node_mut(new_child_ids.1).next_sibling = Some(old_child_ids.0);
        }

        self.node().children = Some((new_child_ids.0, old_child_ids.1));
    }
}

impl<'a, T: 'a> From<NodeMut<'a, T>> for NodeRef<'a, T> {
    fn from(node: NodeMut<'a, T>) -> Self {
        unsafe { node.tree.get_unchecked(node.id) }
    }
}

/// Iterators.
pub mod iter;

/// Creates a tree from expressions.
///
/// # Examples
///
/// ```
/// #[macro_use] extern crate ego_tree;
/// # fn main() {
/// let tree = tree!("root");
/// # }
/// ```
///
/// ```
/// #[macro_use] extern crate ego_tree;
/// # fn main() {
/// let tree = tree! {
///     "root" => {
///         "child a",
///         "child b" => {
///             "grandchild a",
///             "grandchild b",
///         },
///         "child c",
///     }
/// };
/// # }
/// ```
#[macro_export]
macro_rules! tree {
    (@ $n:ident { }) => { };

    // Last leaf.
    (@ $n:ident { $value:expr }) => {
        { $n.append($value); }
    };

    // Leaf.
    (@ $n:ident { $value:expr, $($tail:tt)* }) => {
        {
            $n.append($value);
            tree!(@ $n { $($tail)* });
        }
    };

    // Last node with children.
    (@ $n:ident { $value:expr => $children:tt }) => {
        {
            let mut node = $n.append($value);
            tree!(@ node $children);
        }
    };

    // Node with children.
    (@ $n:ident { $value:expr => $children:tt, $($tail:tt)* }) => {
        {
            {
                let mut node = $n.append($value);
                tree!(@ node $children);
            }
            tree!(@ $n { $($tail)* });
        }
    };

    ($root:expr) => { $crate::Tree::new($root) };

    ($root:expr => $children:tt) => {
        {
            let mut tree = $crate::Tree::new($root);
            {
                let mut node = tree.root_mut();
                tree!(@ node $children);
            }
            tree
        }
    };
}

impl<T: Debug> Debug for Tree<T> {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        use crate::iter::Edge;
        if f.alternate() {
            write!(f, "Tree {{")?;
            for edge in self.root().traverse() {
                match edge {
                    Edge::Open(node) if node.has_children() => {
                        write!(f, " {:?} => {{", node.value())?;
                    }
                    Edge::Open(node) if node.next_sibling().is_some() => {
                        write!(f, " {:?},", node.value())?;
                    }
                    Edge::Open(node) => {
                        write!(f, " {:?}", node.value())?;
                    }
                    Edge::Close(node) if node.has_children() => {
                        if node.next_sibling().is_some() {
                            write!(f, " }},")?;
                        } else {
                            write!(f, " }}")?;
                        }
                    }
                    _ => {}
                }
            }
            write!(f, " }}")
        } else {
            f.debug_struct("Tree").field("vec", &self.vec).finish()
        }
    }
}

// Handles display
mod display;

impl<T: Display> Display for Tree<T> {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        use crate::display::Indentation;
        use crate::iter::Edge;

        let mut indent: Indentation = Indentation::new(true);

        for edge in self.root().traverse() {
            match edge {
                Edge::Open(node) if node.has_children() => {
                    indent.indent(node.next_sibling().is_some());
                    writeln!(f, "{indent}{}", node.value())?;
                }
                Edge::Open(node) => {
                    indent.indent(node.next_sibling().is_some());
                    writeln!(f, "{indent}{}", node.value())?;
                    indent.deindent();
                }
                Edge::Close(node) if node.has_children() => {
                    indent.deindent();
                }
                _ => {}
            }
        }
        Ok(())
    }
}

mod sort;
