// Copyright 2016 Hroi Sigurdsson
//
// Licensed under the MIT license <LICENSE-MIT or http://opensource.org/licenses/MIT>.
// This file may not be copied, modified, or distributed except according to those terms.

#[cfg(feature = "alloc")]
use alloc::vec;
#[cfg(feature = "alloc")]
use alloc::vec::Vec;
use std::cmp;

mod allocator;
mod node;

use self::allocator::{Allocator, AllocatorHandle};
use self::node::{MatchResult, Node};
use std::ptr;

// #[derive(Debug)]
pub struct TreeBitmap<T: Sized> {
    trienodes: Allocator<Node>,
    results: Allocator<T>,
    len: usize,
    should_drop: bool, // drop contents on drop?
}

impl<T: Sized> TreeBitmap<T> {
    /// Returns ````TreeBitmap ```` with 0 start capacity.
    pub fn new() -> Self {
        Self::with_capacity(0)
    }

    /// Returns ```TreeBitmap``` with pre-allocated buffers of size n.
    pub fn with_capacity(n: usize) -> Self {
        let mut trieallocator: Allocator<Node> = Allocator::with_capacity(n);
        let mut root_hdl = trieallocator.alloc(0);
        trieallocator.insert(&mut root_hdl, 0, Node::new());

        TreeBitmap {
            trienodes: trieallocator,
            results: Allocator::with_capacity(n),
            len: 0,
            should_drop: true,
        }
    }

    /// Returns handle to root node.
    fn root_handle(&self) -> AllocatorHandle {
        AllocatorHandle::generate(1, 0)
    }

    /// Returns the root node.
    #[cfg(test)]
    #[allow(dead_code)]
    fn root_node(&self) -> Node {
        *self.trienodes.get(&self.root_handle(), 0)
    }

    /// Push down results encoded in the last 16 bits into child trie nodes.
    /// Makes ```node``` into a normal node.
    fn push_down(&mut self, node: &mut Node) {
        debug_assert!(node.is_endnode(), "push_down: not an endnode");
        debug_assert!(node.child_ptr == 0);
        // count number of internal nodes in the first 15 bits (those that will
        // remain in place).
        let internal_node_count = (node.internal() & 0xffff_0000).count_ones();
        let remove_at = internal_node_count;
        // count how many nodes to push down
        // let nodes_to_pushdown = (node.bitmap & 0x0000_ffff).count_ones();
        let nodes_to_pushdown = (node.internal() & 0x0000_ffff).count_ones();
        if nodes_to_pushdown > 0 {
            let mut result_hdl = node.result_handle();
            let mut child_node_hdl = self.trienodes.alloc(0);

            for _ in 0..nodes_to_pushdown {
                // allocate space for child result value
                let mut child_result_hdl = self.results.alloc(0);
                // put result value in freshly allocated bucket
                let result_value = self.results.remove(&mut result_hdl, remove_at);
                self.results.insert(&mut child_result_hdl, 0, result_value);
                // create and save child node
                let mut child_node = Node::new();
                child_node.set_internal(1 << 31);
                child_node.result_ptr = child_result_hdl.offset;
                // append trienode to collection
                let insert_node_at = child_node_hdl.len;
                self.trienodes
                    .insert(&mut child_node_hdl, insert_node_at, child_node);
            }
            // the result data may have moved to a smaller bucket, update the
            // result pointer
            node.result_ptr = result_hdl.offset;
            node.child_ptr = child_node_hdl.offset;
            // no results from this node remain, free the result slot
            if internal_node_count == 0 && nodes_to_pushdown > 0 {
                self.results.free(&mut result_hdl);
                node.result_ptr = 0;
            }
        }
        node.make_normalnode();
        // note: we do not need to touch the external bits
    }

    fn longest_match_internal(&self, nibbles: &[u8]) -> Option<(AllocatorHandle, u32, u32)> {
        let mut cur_hdl = self.root_handle();
        let mut cur_index = 0;
        let mut bits_searched = 0;
        let mut best_match = None; // result handle + index + bites matched

        let mut loop_count = 0;
        loop {
            let nibble = if loop_count < nibbles.len() {
                nibbles[loop_count]
            } else {
                0
            };
            loop_count += 1;

            let nibble = &nibble;
            let cur_node = *self.trienodes.get(&cur_hdl, cur_index);
            let match_mask = node::MATCH_MASKS[*nibble as usize];

            if let MatchResult::Match(result_hdl, result_index, matching_bit_index) =
                cur_node.match_internal(match_mask)
            {
                let bits_matched = bits_searched + node::BIT_MATCH[matching_bit_index as usize];
                best_match = Some((result_hdl, result_index, bits_matched));
            }

            if cur_node.is_endnode() {
                break;
            }
            match cur_node.match_external(match_mask) {
                MatchResult::Chase(child_hdl, child_index) => {
                    bits_searched += 4;
                    cur_hdl = child_hdl;
                    cur_index = child_index;
                }
                MatchResult::None => {
                    break;
                }
                _ => unreachable!(),
            }
        }

        best_match
    }

    /// longest match lookup of ```nibbles```. Returns bits matched as u32, and reference to T.
    pub fn longest_match(&self, nibbles: &[u8]) -> Option<(u32, &T)> {
        match self.longest_match_internal(&nibbles) {
            Some((result_hdl, result_index, bits_matched)) => {
                Some((bits_matched, self.results.get(&result_hdl, result_index)))
            }
            None => None,
        }
    }

    /// longest match lookup of ```nibbles```. Returns bits matched as u32, and mutable reference to T.
    pub fn longest_match_mut(&mut self, nibbles: &[u8]) -> Option<(u32, &mut T)> {
        match self.longest_match_internal(&nibbles) {
            Some((result_hdl, result_index, bits_matched)) => Some((
                bits_matched,
                self.results.get_mut(&result_hdl, result_index),
            )),
            None => None,
        }
    }

    /// All matches lookup of ```nibbles```. Returns of Vec of tuples, each containing bits matched as u32,
    /// result handle and result index as u32.
    fn matches_internal(&self, nibbles: &[u8]) -> Vec<(u32, AllocatorHandle, u32)> {
        let mut cur_hdl = self.root_handle();
        let mut cur_index = 0;
        let mut bits_searched = 0;
        let mut matches = Vec::new();

        let mut loop_count = 0;
        loop {
            let nibble = if loop_count < nibbles.len() {
                nibbles[loop_count]
            } else {
                0
            };
            loop_count += 1;
            let nibble = &nibble;
            let cur_node = *self.trienodes.get(&cur_hdl, cur_index);

            for i in 0..5 {
                let prefix = *nibble & (!0 << (4 - i));
                let bitmap = node::gen_bitmap(prefix, i as u32) & node::END_BIT_MASK;
                if let MatchResult::Match(result_hdl, result_index, _) =
                    cur_node.match_internal(bitmap)
                {
                    let bits_matched = bits_searched + (i as u32);
                    matches.push((bits_matched, result_hdl, result_index));
                }
            }

            if cur_node.is_endnode() {
                break;
            }

            let match_mask = node::MATCH_MASKS[*nibble as usize];
            match cur_node.match_external(match_mask) {
                MatchResult::Chase(child_hdl, child_index) => {
                    bits_searched += 4;
                    cur_hdl = child_hdl;
                    cur_index = child_index;
                }
                MatchResult::None => {
                    break;
                }
                _ => unreachable!(),
            }
        }

        matches
    }

    /// All matches lookup of ```nibbles```. Returns of iterator of tuples, each containing bits matched as u32 and a reference to T.
    pub fn matches(&self, nibbles: &[u8]) -> impl Iterator<Item = (u32, &T)> {
        self.matches_internal(nibbles).into_iter().map(
            move |(bits_matched, result_hdl, result_index)| {
                (bits_matched, self.results.get(&result_hdl, result_index))
            },
        )
    }

    /// All matches lookup of ```nibbles```. Returns of iterator of tuples, each containing bits matched as u32 and a mutable reference to T.
    pub fn matches_mut(&mut self, nibbles: &[u8]) -> MatchesMut<T> {
        let path = self.matches_internal(nibbles).into_iter();
        MatchesMut { inner: self, path }
    }

    pub fn insert(&mut self, nibbles: &[u8], masklen: u32, value: T) -> Option<T> {
        let mut cur_hdl = self.root_handle();
        let mut cur_index = 0;
        let mut bits_left = masklen;
        let mut ret = None;

        let mut loop_count = 0;
        loop {
            let nibble = if loop_count < nibbles.len() {
                nibbles[loop_count]
            } else {
                0
            };
            loop_count += 1;
            let nibble = &nibble;

            let mut cur_node = *self.trienodes.get(&cur_hdl, cur_index);
            let match_result = cur_node.match_segment(*nibble);

            if let MatchResult::Chase(child_hdl, index) = match_result {
                if bits_left >= 4 {
                    // follow existing branch
                    bits_left -= 4;
                    cur_hdl = child_hdl;
                    cur_index = index;
                    continue;
                }
            }

            let bitmap = node::gen_bitmap(*nibble, cmp::min(4, bits_left));

            if (cur_node.is_endnode() && bits_left <= 4) || bits_left <= 3 {
                // final node reached, insert results
                let mut result_hdl = match cur_node.result_count() {
                    0 => self.results.alloc(0),
                    _ => cur_node.result_handle(),
                };
                let result_index = (cur_node.internal()
                    >> (bitmap & node::END_BIT_MASK).trailing_zeros())
                .count_ones();

                if cur_node.internal() & (bitmap & node::END_BIT_MASK) > 0 {
                    // key already exists!
                    ret = Some(self.results.replace(&result_hdl, result_index - 1, value));
                } else {
                    cur_node.set_internal(bitmap & node::END_BIT_MASK);
                    self.results.insert(&mut result_hdl, result_index, value); // add result
                    self.len += 1;
                }
                cur_node.result_ptr = result_hdl.offset;
                self.trienodes.set(&cur_hdl, cur_index, cur_node); // save trie node
                return ret;
            }
            // add a branch

            if cur_node.is_endnode() {
                // move any result pointers out of the way, so we can add branch
                self.push_down(&mut cur_node);
            }
            let mut child_hdl = match cur_node.child_count() {
                0 => self.trienodes.alloc(0),
                _ => cur_node.child_handle(),
            };

            let child_index = (cur_node.external() >> bitmap.trailing_zeros()).count_ones();

            if cur_node.external() & (bitmap & node::END_BIT_MASK) == 0 {
                // no existing branch; create it
                cur_node.set_external(bitmap & node::END_BIT_MASK);
            } else {
                // follow existing branch
                if let MatchResult::Chase(child_hdl, index) = cur_node.match_segment(*nibble) {
                    self.trienodes.set(&cur_hdl, cur_index, cur_node); // save trie node
                    bits_left -= 4;
                    cur_hdl = child_hdl;
                    cur_index = index;
                    continue;
                }
                unreachable!()
            }

            // prepare a child node
            let mut child_node = Node::new();
            child_node.make_endnode();
            self.trienodes
                .insert(&mut child_hdl, child_index, child_node); // save child
            cur_node.child_ptr = child_hdl.offset;
            self.trienodes.set(&cur_hdl, cur_index, cur_node); // save trie node

            bits_left -= 4;
            cur_hdl = child_hdl;
            cur_index = child_index;
        }
    }

    pub fn mem_usage(&self) -> (usize, usize) {
        let node_bytes = self.trienodes.mem_usage();
        let result_bytes = self.results.mem_usage();
        (node_bytes, result_bytes)
    }

    pub fn len(&self) -> usize {
        self.len
    }

    fn exact_match_internal(&self, nibbles: &[u8], masklen: u32) -> Option<(AllocatorHandle, u32)> {
        let mut cur_hdl = self.root_handle();
        let mut cur_index = 0;
        let mut bits_left = masklen;

        let mut loop_count = 0;
        loop {
            let nibble = if loop_count < nibbles.len() {
                nibbles[loop_count]
            } else {
                0
            };
            loop_count += 1;

            let nibble = &nibble;
            let cur_node = self.trienodes.get(&cur_hdl, cur_index);
            let bitmap = node::gen_bitmap(*nibble, cmp::min(bits_left, 4)) & node::END_BIT_MASK;
            let reached_final_node = bits_left < 4 || (cur_node.is_endnode() && bits_left == 4);

            if reached_final_node {
                return match cur_node.match_internal(bitmap) {
                    MatchResult::Match(result_hdl, result_index, _) => {
                        Some((result_hdl, result_index))
                    }
                    _ => None,
                }
            }

            match cur_node.match_external(bitmap) {
                MatchResult::Chase(child_hdl, child_index) => {
                    cur_hdl = child_hdl;
                    cur_index = child_index;
                    bits_left -= 4;
                }
                _ => return None,
            }
        }
    }

    pub fn exact_match(&self, nibbles: &[u8], masklen: u32) -> Option<&T> {
        self.exact_match_internal(nibbles, masklen).map(move |(result_hdl, result_index)| {
            self.results.get(&result_hdl, result_index)
        })
    }

    pub fn exact_match_mut(&mut self, nibbles: &[u8], masklen: u32) -> Option<&mut T> {
        self.exact_match_internal(nibbles, masklen).map(move |(result_hdl, result_index)| {
            self.results.get_mut(&result_hdl, result_index)
        })
    }

    /// Remove prefix. Returns existing value if the prefix previously existed.
    pub fn remove(&mut self, nibbles: &[u8], masklen: u32) -> Option<T> {
        debug_assert!(nibbles.len() >= (masklen / 4) as usize);
        let root_hdl = self.root_handle();
        let mut root_node = *self.trienodes.get(&root_hdl, 0);
        let ret = self.remove_child(&mut root_node, nibbles, masklen);
        self.trienodes.set(&root_hdl, 0, root_node);
        ret
    }

    // remove child and result from node
    fn remove_child(&mut self, node: &mut Node, nibbles: &[u8], masklen: u32) -> Option<T> {
        let nibble = if masklen > 0 { nibbles[0] } else { 0 };
        let bitmap = node::gen_bitmap(nibble, cmp::min(masklen, 4)) & node::END_BIT_MASK;
        let reached_final_node = masklen < 4 || (node.is_endnode() && masklen == 4);

        if reached_final_node {
            match node.match_internal(bitmap) {
                MatchResult::Match(mut result_hdl, result_index, _) => {
                    node.unset_internal(bitmap);
                    let ret = self.results.remove(&mut result_hdl, result_index);
                    if node.result_count() == 0 {
                        self.results.free(&mut result_hdl);
                    }
                    node.result_ptr = result_hdl.offset;
                    self.len -= 1;
                    return Some(ret);
                }
                _ => return None,
            }
        }

        if let MatchResult::Chase(mut child_node_hdl, index) = node.match_external(bitmap) {
            let mut child_node = *self.trienodes.get(&child_node_hdl, index);
            let ret = self.remove_child(&mut child_node, &nibbles[1..], masklen - 4);

            if child_node.child_count() == 0 && !child_node.is_endnode() {
                child_node.make_endnode();
            }
            if child_node.is_empty() {
                self.trienodes.remove(&mut child_node_hdl, index);
                node.unset_external(bitmap);
                if child_node_hdl.len == 0 {
                    // no child nodes
                    self.trienodes.free(&mut child_node_hdl);
                }
                node.child_ptr = child_node_hdl.offset;
            } else {
                node.child_ptr = child_node_hdl.offset;
                self.trienodes.set(&child_node_hdl, index, child_node);
            }
            ret
        } else {
            None
        }
    }

    pub fn iter(&self) -> Iter<T> {
        let root_hdl = self.root_handle();
        let root_node = *self.trienodes.get(&root_hdl, 0);
        Iter {
            inner: self,
            path: vec![PathElem {
                node: root_node,
                pos: 0,
            }],
            nibbles: vec![0],
        }
    }

    pub fn iter_mut(&mut self) -> IterMut<T> {
        let root_hdl = self.root_handle();
        let root_node = *self.trienodes.get(&root_hdl, 0);
        IterMut {
            inner: self,
            path: vec![PathElem {
                node: root_node,
                pos: 0,
            }],
            nibbles: vec![0],
        }
    }
}

#[derive(Debug)]
struct PathElem {
    node: Node,
    pos: usize,
}

pub struct Iter<'a, T: 'a> {
    inner: &'a TreeBitmap<T>,
    path: Vec<PathElem>,
    nibbles: Vec<u8>,
}

pub struct IterMut<'a, T: 'a> {
    inner: &'a mut TreeBitmap<T>,
    path: Vec<PathElem>,
    nibbles: Vec<u8>,
}

#[cfg_attr(rustfmt, rustfmt_skip)]
static PREFIX_OF_BIT: [u8; 32] = [// 0       1       2      3        4       5       6       7
                                  0b0000, 0b0000, 0b1000, 0b0000, 0b0100, 0b1000, 0b1100, 0b0000,
                                  // 8       9      10      11      12      13      14      15
                                  0b0010, 0b0100, 0b0110, 0b1000, 0b1010, 0b1100, 0b1110,      0,
                                  // 16      17      18      19      20      21      22      23
                                  0b0000, 0b0001, 0b0010, 0b0011, 0b0100, 0b0101, 0b0110, 0b0111,
                                  // 24      25      26      27      28      29      30      31
                                  0b1000, 0b1001, 0b1010, 0b1011, 0b1100, 0b1101, 0b1110, 0b1111];

fn next<T: Sized>(
    trie: &TreeBitmap<T>,
    path: &mut Vec<PathElem>,
    nibbles: &mut Vec<u8>,
) -> Option<(Vec<u8>, u32, AllocatorHandle, u32)> {
    loop {
        let mut path_elem = match path.pop() {
            Some(elem) => elem,
            None => return None,
        };
        let cur_node = path_elem.node;
        let mut cur_pos = path_elem.pos;
        nibbles.pop();
        // optim:
        if cur_pos == 0 && cur_node.result_count() == 0 {
            path_elem.pos = 16;
            cur_pos = 16;
        }
        if path_elem.pos == 32 {
            continue;
        }
        let nibble = PREFIX_OF_BIT[path_elem.pos];
        let bitmap = 1 << (31 - path_elem.pos);

        path_elem.pos += 1;
        nibbles.push(nibble);
        path.push(path_elem);
        // match internal
        if cur_pos < 16 || cur_node.is_endnode() {
            let match_result = cur_node.match_internal(bitmap);
            if let MatchResult::Match(result_hdl, result_index, matching_bit) = match_result {
                let bits_matched =
                    ((path.len() as u32) - 1) * 4 + node::BIT_MATCH[matching_bit as usize];
                return Some((nibbles.clone(), bits_matched, result_hdl, result_index));
            }
        } else if let MatchResult::Chase(child_hdl, child_index) = cur_node.match_external(bitmap) {
            let child_node = trie.trienodes.get(&child_hdl, child_index);
            nibbles.push(0);
            path.push(PathElem {
                node: *child_node,
                pos: 0,
            });
        }
    }
}

impl<'a, T: 'a> Iterator for Iter<'a, T> {
    type Item = (Vec<u8>, u32, &'a T); //(nibbles, masklen, &T)

    fn next(&mut self) -> Option<Self::Item> {
        match next(self.inner, &mut self.path, &mut self.nibbles) {
            Some((path, bits_matched, hdl, index)) => {
                let value = self.inner.results.get(&hdl, index);
                Some((path, bits_matched, value))
            }
            None => None,
        }
    }
}

impl<'a, T: 'a> Iterator for IterMut<'a, T> {
    type Item = (Vec<u8>, u32, &'a mut T); //(nibbles, masklen, &T)

    fn next(&mut self) -> Option<Self::Item> {
        match next(self.inner, &mut self.path, &mut self.nibbles) {
            Some((path, bits_matched, hdl, index)) => unsafe {
                let ptr: *mut T = self.inner.results.get_mut(&hdl, index);
                let val_ref = &mut *ptr;
                Some((path, bits_matched, val_ref))
            },
            None => None,
        }
    }
}

pub struct IntoIter<T> {
    inner: TreeBitmap<T>,
    path: Vec<PathElem>,
    nibbles: Vec<u8>,
}

impl<'a, T: 'a> Iterator for IntoIter<T> {
    type Item = (Vec<u8>, u32, T); //(nibbles, masklen, T)

    fn next(&mut self) -> Option<Self::Item> {
        match next(&self.inner, &mut self.path, &mut self.nibbles) {
            Some((path, bits_matched, hdl, index)) => {
                let value = self.inner.results.get(&hdl, index);
                let value = unsafe { ptr::read(value) };
                Some((path, bits_matched, value))
            }
            None => None,
        }
    }
}

impl<T> IntoIterator for TreeBitmap<T> {
    type Item = (Vec<u8>, u32, T); //(nibbles, masklen, T)
    type IntoIter = IntoIter<T>;

    fn into_iter(mut self) -> IntoIter<T> {
        let root_hdl = self.root_handle();
        let root_node = *self.trienodes.get(&root_hdl, 0);
        self.should_drop = false; // IntoIter will drop contents
        IntoIter {
            inner: self,
            path: vec![PathElem {
                node: root_node,
                pos: 0,
            }],
            nibbles: vec![0],
        }
    }
}

impl<T> Drop for IntoIter<T> {
    fn drop(&mut self) {
        for _ in self {}
    }
}

pub struct MatchesMut<'a, T: 'a> {
    inner: &'a mut TreeBitmap<T>,
    path: std::vec::IntoIter<(u32, AllocatorHandle, u32)>,
}

impl<'a, T: 'a> Iterator for MatchesMut<'a, T> {
    type Item = (u32, &'a mut T);

    fn next(&mut self) -> Option<Self::Item> {
        match self.path.next() {
            Some((bits_matched, hdl, index)) => unsafe {
                let ptr: *mut T = self.inner.results.get_mut(&hdl, index);
                let val_ref = &mut *ptr;
                Some((bits_matched, val_ref))
            },
            None => None,
        }
    }
}

impl<T> Drop for TreeBitmap<T> {
    fn drop(&mut self) {
        if self.should_drop {
            for (_, _, item) in self.iter() {
                unsafe {
                    ptr::read(item);
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn len() {
        let mut tbm: TreeBitmap<&str> = TreeBitmap::new();
        assert_eq!(tbm.len(), 0);

        let (nibbles_a, mask_a) = (&[0, 10, 0, 0, 0, 0, 0, 0], 8);
        let (nibbles_b, mask_b) = (&[0, 10, 0, 10, 0, 10, 0, 0], 24);

        tbm.insert(nibbles_a, mask_a, "foo");
        assert_eq!(tbm.len(), 1);

        // Insert same nibbles again
        tbm.insert(nibbles_a, mask_a, "foo2");
        assert_eq!(tbm.len(), 1);

        tbm.insert(nibbles_b, mask_b, "bar");
        assert_eq!(tbm.len(), 2);

        tbm.remove(nibbles_b, mask_b);
        assert_eq!(tbm.len(), 1);

        tbm.remove(nibbles_a, mask_a);
        assert_eq!(tbm.len(), 0);
    }

    #[test]
    fn remove() {
        let mut tbm: TreeBitmap<&str> = TreeBitmap::new();
        let (nibbles_a, mask_a) = (&[0, 10, 0, 0, 0, 0, 0, 0], 8);
        let (nibbles_b, mask_b) = (&[0, 10, 0, 10, 0, 10, 0, 0], 24);
        tbm.insert(nibbles_a, mask_a, "foo");
        tbm.insert(nibbles_b, mask_b, "bar");

        {
            let mut matches = tbm.matches(nibbles_b);
            assert_eq!(
                true,
                matches.any(|p| p == (mask_a, &"foo")) && matches.any(|p| p == (mask_b, &"bar"))
            );
        }

        {
            let value = tbm.remove(nibbles_b, mask_b);
            assert_eq!(value, Some("bar"));
            let lookup_result = tbm.longest_match(nibbles_b);
            assert_eq!(lookup_result, Some((mask_a, &"foo")));
        }
        // foo should not exist, and therefore return None
        let value = tbm.remove(nibbles_b, mask_b);
        assert_eq!(value, None);
    }

    #[test]
    fn iter() {
        let mut tbm: TreeBitmap<u32> = TreeBitmap::new();
        let (nibbles_a, mask_a) = (&[0], 0);
        let (nibbles_b, mask_b) = (&[0, 10], 8);
        let (nibbles_c, mask_c) = (&[0, 10, 0, 10, 0, 10], 24);
        let (nibbles_d, mask_d) = (&[0, 10, 0, 10, 1, 11], 24);
        tbm.insert(nibbles_a, mask_a, 1);
        tbm.insert(nibbles_b, mask_b, 2);
        tbm.insert(nibbles_c, mask_c, 3);
        tbm.insert(nibbles_d, mask_d, 4);

        let mut iter = tbm.iter();
        assert_eq!(iter.next().unwrap().2, &1);
        assert_eq!(iter.next().unwrap().2, &2);
        assert_eq!(iter.next().unwrap().2, &3);
        assert_eq!(iter.next().unwrap().2, &4);
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn into_iter() {
        let mut tbm: TreeBitmap<u32> = TreeBitmap::new();
        let (nibbles_a, mask_a) = (&[0], 0);
        let (nibbles_b, mask_b) = (&[0, 10], 8);
        let (nibbles_c, mask_c) = (&[0, 10, 0, 10, 0, 10], 24);
        let (nibbles_d, mask_d) = (&[0, 10, 0, 10, 1, 11], 24);
        tbm.insert(nibbles_a, mask_a, 1);
        tbm.insert(nibbles_b, mask_b, 2);
        tbm.insert(nibbles_c, mask_c, 3);
        tbm.insert(nibbles_d, mask_d, 4);

        let mut iter = tbm.into_iter();
        assert_eq!(iter.next().unwrap().2, 1);
        assert_eq!(iter.next().unwrap().2, 2);
        assert_eq!(iter.next().unwrap().2, 3);
        assert_eq!(iter.next().unwrap().2, 4);
        assert_eq!(iter.next(), None);
    }

    struct Thing {
        id: usize,
    }
    impl Drop for Thing {
        fn drop(&mut self) {
            println!("dropping id {}", self.id);
        }
    }

    #[test]
    fn drop() {
        let mut tbm: TreeBitmap<Thing> = TreeBitmap::new();
        let (nibbles_a, mask_a) = (&[0], 0);
        let (nibbles_b, mask_b) = (&[0, 10], 8);
        let (nibbles_c, mask_c) = (&[0, 10, 0, 10, 0, 10], 24);
        let (nibbles_d, mask_d) = (&[0, 10, 0, 10, 1, 11], 24);
        tbm.insert(nibbles_a, mask_a, Thing { id: 1 });
        tbm.insert(nibbles_b, mask_b, Thing { id: 2 });
        tbm.insert(nibbles_c, mask_c, Thing { id: 3 });
        tbm.insert(nibbles_d, mask_d, Thing { id: 4 });
        println!("should drop");
    }

    #[test]
    fn into_iter_drop() {
        let mut tbm: TreeBitmap<Thing> = TreeBitmap::new();
        let (nibbles_a, mask_a) = (&[0], 0);
        let (nibbles_b, mask_b) = (&[0, 10], 8);
        let (nibbles_c, mask_c) = (&[0, 10, 0, 10, 0, 10], 24);
        let (nibbles_d, mask_d) = (&[0, 10, 0, 10, 1, 11], 24);
        tbm.insert(nibbles_a, mask_a, Thing { id: 1 });
        tbm.insert(nibbles_b, mask_b, Thing { id: 2 });
        tbm.insert(nibbles_c, mask_c, Thing { id: 3 });
        tbm.insert(nibbles_d, mask_d, Thing { id: 4 });
        let mut iter = tbm.into_iter();
        iter.next();
        iter.next();
        println!("should drop 3 - 4");
    }
}
