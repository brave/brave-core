// Copyright 2016 Hroi Sigurdsson
//
// Licensed under the MIT license <LICENSE-MIT or http://opensource.org/licenses/MIT>.
// This file may not be copied, modified, or distributed except according to those terms.

use super::allocator::AllocatorHandle;
#[cfg(feature = "alloc")]
use alloc::format;
#[cfg(feature = "alloc")]
use alloc::vec::Vec;

pub const INT_MASK: u32 = 0xffff_0000;
pub const EXT_MASK: u32 = 0x0000_ffff;
pub const END_BIT: u32 = 1 << 16;
pub const END_BIT_MASK: u32 = !END_BIT; // all bits except the end node bit

type Table = [[u32; 16]; 5];
const IS_END_NODE: u32 = 1 << 16;

#[cfg_attr(rustfmt, rustfmt_skip)]
static INTERNAL_LOOKUP_TABLE: Table = [
    // mask = 00000, 0/0
    [1<<31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
    // mask = 00001, 0-1/1
    [1<<30, 0, 0, 0, 0, 0, 0, 0, 1<<29, 0, 0, 0, 0, 0, 0, 0],
    // mask = 00011, 0-3/2
    [1<<28, 0, 0, 0, 1<<27, 0, 0, 0, 1<<26, 0, 0, 0, 1<<25, 0, 0, 0],
    // mask = 00111, 0-7/3
    [1<<24, 0, 1<<23, 0, 1<<22, 0, 1<<21, 0, 1<<20, 0, 1<<19, 0, 1<<18, 0, 1<<17, 0],
    // endnode indicated in 16 bit, skip
    // mask = 01111, 0-15/4
    [IS_END_NODE | 1<<15, IS_END_NODE | 1<<14, IS_END_NODE | 1<<13, IS_END_NODE | 1<<12,
     IS_END_NODE | 1<<11, IS_END_NODE | 1<<10, IS_END_NODE | 1<< 9, IS_END_NODE | 1<< 8,
     IS_END_NODE | 1<< 7, IS_END_NODE | 1<< 6, IS_END_NODE | 1<< 5, IS_END_NODE | 1<< 4,
     IS_END_NODE | 1<< 3, IS_END_NODE | 1<< 2, IS_END_NODE | 1<< 1, IS_END_NODE |     1],
];

pub const MSB: u32 = 1 << 31;

#[cfg_attr(rustfmt, rustfmt_skip)]
pub static MATCH_MASKS: [u32; 16] = [MSB | MSB >> 1 | MSB >> 3 | MSB >>  7 | MSB >> 16, // 0000
                                     MSB | MSB >> 1 | MSB >> 3 | MSB >>  7 | MSB >> 17, // 0001
                                     MSB | MSB >> 1 | MSB >> 3 | MSB >>  8 | MSB >> 18, // 0010
                                     MSB | MSB >> 1 | MSB >> 3 | MSB >>  8 | MSB >> 19, // 0011

                                     MSB | MSB >> 1 | MSB >> 4 | MSB >>  9 | MSB >> 20, // 0100
                                     MSB | MSB >> 1 | MSB >> 4 | MSB >>  9 | MSB >> 21, // 0101
                                     MSB | MSB >> 1 | MSB >> 4 | MSB >> 10 | MSB >> 22, // 0110
                                     MSB | MSB >> 1 | MSB >> 4 | MSB >> 10 | MSB >> 23, // 0111

                                     MSB | MSB >> 2 | MSB >> 5 | MSB >> 11 | MSB >> 24, // 1000
                                     MSB | MSB >> 2 | MSB >> 5 | MSB >> 11 | MSB >> 25, // 1001
                                     MSB | MSB >> 2 | MSB >> 5 | MSB >> 12 | MSB >> 26, // 1010
                                     MSB | MSB >> 2 | MSB >> 5 | MSB >> 12 | MSB >> 27, // 1011

                                     MSB | MSB >> 2 | MSB >> 6 | MSB >> 13 | MSB >> 28, // 1100
                                     MSB | MSB >> 2 | MSB >> 6 | MSB >> 13 | MSB >> 29, // 1101
                                     MSB | MSB >> 2 | MSB >> 6 | MSB >> 14 | MSB >> 30, // 1110
                                     MSB | MSB >> 2 | MSB >> 6 | MSB >> 14 | MSB >> 31  /* 1111 */];

#[inline]
pub fn gen_bitmap(prefix: u8, masklen: u32) -> u32 {
    debug_assert!(prefix < 16); // only nibbles allowed
    debug_assert!(masklen < 5);
    let ret = INTERNAL_LOOKUP_TABLE[masklen as usize][prefix as usize];
    assert!(ret > 0, "ret > 0, maybe IP network has host bits set");
    ret
}

/// ```Node ``` encodes result and child node pointers in a bitmap.
///
/// A trie node can encode up to 31 results when acting as an "end node", or 16
/// results and 16 children/subtrees.
///
/// Each bit in the bitmap has the following meanings:
///
/// | bit   | 0 |  1 |  2 |  3  |   4 |   5 |   6 |    7 |
/// |-------|---|----|----|-----|-----|-----|-----|------|
/// | match | * | 0* | 1* | 00* | 01* | 10* | 11* | 000* |
///
/// | bit   |    8 |    9 |   10 |   11 |   12 |   13 |   14 |          15 |
/// |-------|------|------|------|------|------|------|------|-------------|
/// | match | 001* | 010* | 011* | 100* | 101* | 110* | 111* | endnode-bit |

/// If the end node bit is set, the last bits are also used to match internal
/// nodes:
///
/// | bit   |    16 |    17 |    18 |    19 |    20 |    21 |    22 |    23 |
/// |-------|-------|-------|-------|-------|-------|-------|-------|-------|
/// | match | 0000* | 0001* | 0010* | 0011* | 0100* | 0101* | 0110* | 0111* |
///
/// | bit   |    24 |    25 |    26 |    27 |    28 |    29 |    30 |    31 |
/// |-------|-------|-------|-------|-------|-------|-------|-------|-------|
/// | match | 1000* | 1001* | 1010* | 1011* | 1100* | 1101* | 1110* | 1111* |

/// The location of the result value is computed with the ```result_ptr``` base
/// pointer and the number of bits set left of the matching bit.
///
/// If the endnode bit is not set, the last 16 bits encodes pointers to child
/// nodes.
/// If bit N is set it means that a child node with segment value N is present.
/// The pointer to the child node is then computed with the ```child_ptr``` base
/// pointer and the number of bits set left of N.
#[derive(Clone, Copy)]
pub struct Node {
    /// child/result bitmap
    bitmap: u32, // first 16 bits: internal, last 16 bits: child bitmap
    /// child base pointer
    pub child_ptr: u32,
    /// results base pointer
    pub result_ptr: u32,
}

pub const BIT_MATCH: [u32; 32] = [
    0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
];

#[cfg_attr(rustfmt, rustfmt_skip)]
const BIT_MEANING: &[&str] = &[
    "*",
    "0*", "1*",
    "00*", "01*", "10*", "11*",
    "000*", "001*", "010*", "011*", "100*", "101*", "110*", "111*",
    "END",
    "0000*", "0001*", "0010*", "0011*", "0100*", "0101*", "0110*", "0111*",
    "1000*", "1001*", "1010*", "1011*", "1100*", "1101*", "1110*", "1111*",
];

use std::fmt;
impl fmt::Debug for Node {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        let mut int_nodes: Vec<&str> = Vec::new();
        let mut child_nodes: Vec<u32> = Vec::new();
        let mut selector = 1 << 31;
        for meaning in BIT_MEANING {
            if self.internal() & selector > 0 {
                int_nodes.push(meaning);
            }
            selector >>= 1;
        }

        selector = 1 << 15;
        for i in 0..16 {
            if self.external() & selector > 0 {
                child_nodes.push(i);
            }
            selector >>= 1;
        }

        let bitmap_string = format!("{:016b} {:016b}", self.bitmap >> 16, self.bitmap & EXT_MASK);

        if self.is_endnode() {
            return f
                .debug_struct("EndNode")
                .field("bitmap", &bitmap_string)
                .field("internal", &int_nodes)
                .field("result_ptr", &self.result_ptr)
                .finish();
        }
        if self.is_blank() {
            return f.debug_struct("BlankNode").finish();
        }
        f.debug_struct("InternalNode")
            .field("bitmap", &bitmap_string)
            .field("internal", &int_nodes)
            .field("children", &child_nodes)
            .field("child_ptr", &self.child_ptr)
            .field("result_ptr", &self.result_ptr)
            .finish()
    }
}

impl Node {
    /// Get a fresh, blank node.
    pub fn new() -> Self {
        Node {
            bitmap: 0,
            child_ptr: 0,
            result_ptr: 0,
        }
    }

    /// Is node blank?
    pub fn is_blank(&self) -> bool {
        self.bitmap == 0 && self.child_ptr == 0 && self.result_ptr == 0
    }

    /// Is node empty?
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.bitmap & END_BIT_MASK == 0
    }

    /// Is node an end node?
    #[inline]
    pub fn is_endnode(&self) -> bool {
        self.bitmap & END_BIT > 0
    }

    /// Get internal bitmap (result entries). Any external bits are filtered.
    #[inline]
    pub fn internal(&self) -> u32 {
        if self.is_endnode() {
            self.bitmap & END_BIT_MASK // filter the end node bit
        } else {
            self.bitmap & INT_MASK
        }
    }

    /// Get external bitmap (child entries). Any internal bits are filtered.
    #[inline]
    pub fn external(&self) -> u32 {
        if self.is_endnode() {
            0
        } else {
            self.bitmap & EXT_MASK
        }
    }

    /// Set the endnode-bit.
    /// # Panics
    /// + if bit already set.
    /// + if there are any external node pointers.
    #[inline]
    pub fn make_endnode(&mut self) {
        debug_assert!(!self.is_endnode(), "make_endnode: already an endnode.");
        debug_assert!(
            self.external() == 0,
            "cannot make into endnode when there are children present"
        );
        self.bitmap |= END_BIT
    }

    /// Unset the endnode-bit.
    /// # Panics
    /// + if not already an endnode.
    #[inline]
    pub fn make_normalnode(&mut self) {
        debug_assert!(self.is_endnode(), "make_endnode: already a normalnode.");
        self.bitmap &= END_BIT_MASK
    }

    /// Get number of child pointers.
    #[inline]
    pub fn child_count(&self) -> u32 {
        self.external().count_ones()
    }

    /// Get number of result pointers.
    #[inline]
    pub fn result_count(&self) -> u32 {
        self.internal().count_ones()
    }

    /// Get handle to the results.
    #[inline]
    pub fn result_handle(&self) -> AllocatorHandle {
        AllocatorHandle::generate(self.result_count(), self.result_ptr)
    }

    /// Get handle to child nodes.
    #[inline]
    pub fn child_handle(&self) -> AllocatorHandle {
        AllocatorHandle::generate(self.child_count(), self.child_ptr)
    }

    /// Set an internal bit.
    #[inline]
    pub fn set_internal(&mut self, bitmap: u32) {
        debug_assert!(
            bitmap.count_ones() == 1,
            "set_internal: bitmap must contain exactly one bit"
        );
        debug_assert!(
            bitmap & END_BIT == 0,
            "set_internal: not permitted to set the endnode bit"
        );
        debug_assert!(self.bitmap & bitmap == 0, "set_internal: bit already set");
        if !self.is_endnode() {
            debug_assert!(
                bitmap & EXT_MASK == 0,
                "set_internal: attempted to set external bit"
            );
        }
        self.bitmap |= bitmap
    }

    /// Unset an internal bit.
    #[inline]
    pub fn unset_internal(&mut self, bitmap: u32) {
        debug_assert!(
            bitmap.count_ones() == 1,
            "unset_internal: bitmap must contain exactly one bit"
        );
        debug_assert!(
            bitmap & END_BIT == 0,
            "unset_internal: not permitted to set the endnode bit"
        );
        debug_assert!(
            self.bitmap & bitmap == bitmap,
            "unset_internal: bit already unset"
        );
        if !self.is_endnode() {
            debug_assert!(
                bitmap & EXT_MASK == 0,
                "unset_internal: attempted to set external bit"
            );
        }
        self.bitmap ^= bitmap
    }

    /// Set an external bit.
    #[inline]
    pub fn set_external(&mut self, bitmap: u32) {
        debug_assert!(
            !self.is_endnode(),
            "set_external: endnodes don't have external bits"
        );
        debug_assert!(
            bitmap & END_BIT == 0,
            "set_external: not permitted to set the endnode bit"
        );
        debug_assert!(
            self.bitmap & bitmap == 0,
            "set_external: not permitted to set an already set bit"
        );
        debug_assert!(
            bitmap & INT_MASK == 0,
            "set_external: not permitted to set an internal bit"
        );
        self.bitmap |= bitmap
    }

    pub fn unset_external(&mut self, bitmap: u32) {
        debug_assert!(
            !self.is_endnode(),
            "unset_external: endnodes don't have external bits"
        );
        debug_assert!(
            bitmap & END_BIT == 0,
            "unset_external: not permitted to set the endnode bit"
        );
        debug_assert!(
            self.bitmap & bitmap == bitmap,
            "unset_external: not permitted to unset an already unset bit"
        );
        debug_assert!(
            bitmap & INT_MASK == 0,
            "unset_external: not permitted to set an internal bit"
        );
        self.bitmap ^= bitmap
    }

    /// Perform a match on segment/masklen.
    #[inline]
    pub fn match_segment(&self, segment: u8) -> MatchResult {
        let match_mask = MATCH_MASKS[segment as usize];
        match self.match_external(match_mask) {
            MatchResult::None => self.match_internal(match_mask),
            x => x,
        }
    }

    #[inline]
    pub fn match_internal(&self, match_mask: u32) -> MatchResult {
        let result_match = self.internal() & match_mask;
        if result_match > 0 {
            let result_hdl = self.result_handle();
            let best_match_bit_index = 31 - result_match.trailing_zeros();
            let best_match_result_index = match best_match_bit_index {
                0 => 0,
                _ => (self.internal() >> (32 - best_match_bit_index)).count_ones(),
            };
            return MatchResult::Match(result_hdl, best_match_result_index, best_match_bit_index);
        }
        MatchResult::None
    }

    #[inline]
    pub fn match_external(&self, match_mask: u32) -> MatchResult {
        let child_match = self.external() & match_mask;
        if child_match > 0 {
            let child_hdl = self.child_handle();
            let best_match_bit_index = 31 - child_match.trailing_zeros();
            let best_match_child_index = match best_match_bit_index {
                0 => 0,
                _ => (self.external() >> (32 - best_match_bit_index)).count_ones(),
            };
            return MatchResult::Chase(child_hdl, best_match_child_index);
        }
        MatchResult::None
    }
}

#[derive(Debug)]
pub enum MatchResult {
    Match(AllocatorHandle, u32, u32), // result_handle, offset, matching bits
    Chase(AllocatorHandle, u32),      // child_handle, offset
    None,                             // Node does not match
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_trienode_new() {
        Node::new();
    }

    #[test]
    fn match_segment() {
        // case 1
        let mut node = Node::new();
        node.make_endnode();
        node.set_internal(MSB >> 2); // 1*
        node.set_internal(MSB >> 4); // 01*
        node.set_internal(MSB >> 9); // 010*
        println!("{:#?}", node);

        let segment = 0b1111;
        let match_result = node.match_segment(segment);
        println!("match_segment({:04b}): {:?}", segment, match_result);
        match match_result {
            MatchResult::Match(_, _, _) => (),
            _ => panic!("match failure"),
        }

        let segment = 0b0011;
        let match_result = node.match_segment(segment);
        println!("match_segment({:04b}): {:?}", segment, match_result);
        match match_result {
            MatchResult::None => {}
            _ => panic!("match failure"),
        }

        let mut node = Node::new();
        node.set_external(MSB >> 23); // 0111*

        let segment = 0b0011;
        let match_result = node.match_segment(segment);
        println!("match_segment({:04b}): {:?}", segment, match_result);
        match match_result {
            MatchResult::None => {}
            _ => panic!("match failure"),
        }

        let segment = 0b0111;
        let match_result = node.match_segment(segment);
        println!("match_segment({:04b}): {:?}", segment, match_result);
        match match_result {
            MatchResult::Chase(_, _) => {}
            _ => panic!("match failure"),
        }
    }
}
