//! Sorting functionality for tree nodes.
//!
//! This module provides methods for sorting children of a node in a tree.
//! The sorting can be done based on the node values or their indices.

use std::cmp::Ordering;

use crate::{NodeMut, NodeRef};

impl<'a, T: 'a> NodeMut<'a, T> {
    /// Sort children by value in ascending order.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use ego_tree::tree;
    ///
    /// let mut tree = tree!('a' => { 'd', 'c', 'b' });
    /// tree.root_mut().sort();
    /// assert_eq!(
    ///     vec![&'b', &'c', &'d'],
    ///     tree.root()
    ///         .children()
    ///         .map(|n| n.value())
    ///         .collect::<Vec<_>>(),
    /// );
    /// ```
    pub fn sort(&mut self)
    where
        T: Ord,
    {
        self.sort_by(|a, b| a.value().cmp(b.value()));
    }

    /// Sort children by `NodeRef` in ascending order using a comparison function.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use ego_tree::tree;
    ///
    /// let mut tree = tree!('a' => { 'c', 'd', 'b' });
    /// tree.root_mut().sort_by(|a, b| b.value().cmp(a.value()));
    /// assert_eq!(
    ///     vec![&'d', &'c', &'b'],
    ///     tree.root()
    ///         .children()
    ///         .map(|n| n.value())
    ///         .collect::<Vec<_>>(),
    /// );
    ///
    /// // Example for sort_by_id.
    /// tree.root_mut().sort_by(|a, b| a.id().cmp(&b.id()));
    /// assert_eq!(
    ///     vec![&'c', &'d', &'b'],
    ///     tree.root()
    ///         .children()
    ///         .map(|n| n.value())
    ///         .collect::<Vec<_>>(),
    /// );
    /// ```
    pub fn sort_by<F>(&mut self, mut compare: F)
    where
        F: FnMut(NodeRef<T>, NodeRef<T>) -> Ordering,
    {
        if !self.has_children() {
            return;
        }

        let mut children = {
            let this = unsafe { self.tree.get_unchecked(self.id) };
            this.children().map(|child| child.id).collect::<Vec<_>>()
        };

        children.sort_by(|a, b| {
            let a = unsafe { self.tree.get_unchecked(*a) };
            let b = unsafe { self.tree.get_unchecked(*b) };
            compare(a, b)
        });

        for id in children {
            self.append_id(id);
        }
    }

    /// Sort children by `NodeRef`'s key in ascending order using a key extraction function.
    ///
    /// # Examples
    ///
    /// ```rust
    /// use ego_tree::tree;
    ///
    /// let mut tree = tree!("1a" => { "2b", "4c", "3d" });
    /// tree.root_mut().sort_by_key(|a| a.value().split_at(1).0.parse::<i32>().unwrap());
    /// assert_eq!(
    ///     vec!["2b", "3d", "4c"],
    ///     tree.root()
    ///         .children()
    ///         .map(|n| *n.value())
    ///         .collect::<Vec<_>>(),
    /// );
    ///
    /// // Example for sort_by_id.
    /// tree.root_mut().sort_by_key(|n| n.id());
    /// assert_eq!(
    ///     vec![&"2b", &"4c", &"3d"],
    ///     tree.root()
    ///         .children()
    ///         .map(|n| n.value())
    ///         .collect::<Vec<_>>(),
    /// );
    /// ```
    pub fn sort_by_key<K, F>(&mut self, mut f: F)
    where
        F: FnMut(NodeRef<T>) -> K,
        K: Ord,
    {
        self.sort_by(|a, b| f(a).cmp(&f(b)));
    }
}
