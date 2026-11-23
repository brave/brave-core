//! Checkpoint support for saving and restoring partial decompression progress.
//!
//! This module provides [`checkpoint()`](super::InflaterManaged::checkpoint) and
//! [`restore_from_checkpoint()`](super::InflaterManaged::restore_from_checkpoint)
//! methods on [`InflaterManaged`](super::InflaterManaged) for persisting inflater
//! state across process restarts. This is useful when decompressing very large
//! streams where a crash or interruption would otherwise lose all progress.
//!
//! # Stability
//!
//! The checkpoint format is experimental and may change between library versions.
//! Checkpoints include an internal version number; `restore_from_checkpoint()`
//! returns `None` for incompatible versions. Do not rely on checkpoints persisting
//! across library upgrades.
//!
//! # Usage
//!
//! Checkpoints are typically saved periodically during decompression:
//!
//! ```ignore
//! let mut bytes_written = 0u64;
//! loop {
//!     let n = input.read(&mut input_buf)?;
//!     if n == 0 && inflater.finished() { break; }
//!
//!     let result = inflater.inflate(&input_buf[..n], &mut output_buf);
//!     output.write_all(&output_buf[..result.bytes_written])?;
//!     bytes_written += result.bytes_written as u64;
//!
//!     // Save checkpoint every 100 MB
//!     if bytes_written % 100_000_000 < result.bytes_written as u64 {
//!         if let Some((data, _)) = inflater.checkpoint() {
//!             std::fs::write("checkpoint.dat", &data)?;
//!         }
//!     }
//! }
//! ```
//!
//! To restore from a checkpoint, call `restore_from_checkpoint()` and seek both
//! streams to the positions indicated by the returned [`CheckpointStreamPositions`](CheckpointStreamPositions):
//!
//! ```ignore
//! let mut inflater = InflaterManaged::new();
//! if let Some(pos) = inflater.restore_from_checkpoint(&checkpoint_data) {
//!     input.seek(SeekFrom::Start(pos.input_bytes_to_skip))?;
//!     output.seek(SeekFrom::Start(pos.output_bytes_already_returned))?;
//!     // Continue with normal inflate() loop
//! }
//! ```
//!
//! # Security
//!
//! Checkpoint data represents internal program state. While `restore_from_checkpoint()`
//! validates checksums and structural integrity, malformed data from untrusted sources
//! could cause decompression errors or incorrect output. Only restore checkpoints you
//! created.
//!
//! # Checkpoint Size
//!
//! Checkpoints are typically around 65KB but can reach 131KB if the inflater has
//! significant buffered output not yet drained by the caller. The format consists
//! of a 346-byte header (version, bit position, block state, Huffman code lengths,
//! output counters), followed by the output window history, and a Fletcher-32
//! checksum.

use crate::huffman_tree::HuffmanTree;
use crate::input_buffer::{BitsBuffer, InputBuffer};
use crate::{BlockType, InflaterState};

use super::{InflaterManaged, TABLE_LOOKUP_DISTANCE_MAX};

// Checkpoint binary format (little-endian):
//
//   Offset  Size  Field
//   ------  ----  ----------------------------------
//   0       2     version: u16 (currently 0x1001)
//   2       8     input_bits: u64
//   10      1     buffered_value: u8 (0-7 unread bits)
//   11      1     bfinal_block_type: u8 ((bfinal << 7) | block_type)
//   12      2     uncompressed_remaining: u16
//   14      288   lit_code_lengths: [u8; 288]
//   302     32    dist_code_lengths: [u8; 32]
//   334     8     output_bytes_written: u64
//   342     4     output_bytes_unread: u32
//   346     var   window_data: [u8] (len = max(min(65538, output_bytes_written), output_bytes_unread))
//   END-4   4     checksum: u32 (Fletcher-32)

const CHECKPOINT_HEADER_SIZE: usize = 346;

fn fletcher32_checksum(data: &[u8]) -> u32 {
    let (mut a, mut b) = (0u32, 0u32);
    for &byte in data {
        a = a.wrapping_add(byte as u32);
        b = b.wrapping_add(a);
    }
    (b << 16) | (a & 0xFFFF)
}

/// Update checkpoint state after a write to the output window or end-of-block.
/// Called from the parent module's decode functions.
#[inline(always)]
pub(super) fn update_checkpoint(
    inflater: &mut InflaterManaged,
    input: &InputBuffer<'_>,
    end_of_block: bool,
) {
    debug_assert!(input.available_bits() >= 0 && input.available_bits() <= 32);
    // checkpoint_input_bits tracks the number of input bits consumed up to the checkpoint.
    inflater.checkpoint_input_bits =
        (inflater.total_input_loaded + input.read_bytes as u64) * 8 - input.available_bits() as u64;
    // checkpoint_bit_buffer holds unconsumed bits of the most recently loaded input byte.
    inflater.checkpoint_bit_buffer = input.peek_available_bits() as u8;
    // checkpoint_bfinal_block_type holds bfinal state and current block type.
    // End-of-block is stored as uncompressed with zero remaining (functionally identical).
    let bfinal_flag = (inflater.bfinal as u8) << 7;
    if end_of_block {
        debug_assert!(matches!(
            inflater.state,
            InflaterState::ReadingBFinal | InflaterState::Done
        ));
        inflater.checkpoint_bfinal_block_type = BlockType::Uncompressed as u8 | bfinal_flag;
    } else {
        match inflater.block_type {
            BlockType::Uncompressed => {
                debug_assert_eq!(inflater.state, InflaterState::DecodingUncompressed);
                debug_assert!(inflater.block_length > 0);
            }
            BlockType::Static => debug_assert_eq!(inflater.state, InflaterState::DecodeTop),
            BlockType::Dynamic => debug_assert_eq!(inflater.state, InflaterState::DecodeTop),
        };
        inflater.checkpoint_bfinal_block_type = inflater.block_type as u8 | bfinal_flag;
    }
}

impl InflaterManaged {
    /// Serialize the most recent inflater checkpoint for use with
    /// [`restore_from_checkpoint()`](Self::restore_from_checkpoint).
    ///
    /// Returns `None` if no checkpoint is available (no data processed yet,
    /// inflater errored, or decompression already complete with output drained).
    ///
    /// The returned checkpoint can contain up to 129KB of data representing the
    /// inflater state and history buffer. The `CheckpointStreamPositions` describes
    /// the input/output byte offsets corresponding to this checkpoint.
    #[cfg_attr(docsrs, doc(cfg(feature = "checkpoint")))]
    pub fn checkpoint(&self) -> Option<(Vec<u8>, CheckpointStreamPositions)> {
        if self.checkpoint_input_bits == 0
            || self.errored()
            || (self.output.available_bytes() == 0 && self.state == InflaterState::Done)
        {
            return None;
        }

        let checkpoint_block_type =
            BlockType::from_int((self.checkpoint_bfinal_block_type & 0x7F) as u16)?;
        let uncompressed_remaining = match checkpoint_block_type {
            BlockType::Uncompressed => self.block_length as u32,
            _ => 0,
        };

        let mut lit_codes = [0; HuffmanTree::MAX_LITERAL_TREE_ELEMENTS];
        let mut dist_codes = [0; HuffmanTree::MAX_DIST_TREE_ELEMENTS];
        if checkpoint_block_type == BlockType::Dynamic {
            let lens = self.literal_length_tree.code_lengths();
            lit_codes[..lens.len()].copy_from_slice(lens);
            let lens = self.distance_tree.code_lengths();
            dist_codes[..lens.len()].copy_from_slice(lens);
        }

        let output_bytes_written =
            self.total_output_consumed + self.output.available_bytes() as u64;
        let bytes_unread = self.output.available_bytes() as u32;
        let (window_a, window_b) = self.output.get_checkpoint_data(output_bytes_written);

        let bfinal_block_type = self.checkpoint_bfinal_block_type;

        // Mask unreferenced high bits for deterministic serialization
        let num_buffered_bits = (8 - (self.checkpoint_input_bits & 7)) as u32 & 7;
        let buffered_value = self.checkpoint_bit_buffer & ((1 << num_buffered_bits) - 1);

        let mut out = Vec::with_capacity(CHECKPOINT_HEADER_SIZE + window_a.len() + window_b.len());
        out.extend_from_slice(&0x1001u16.to_le_bytes()); // version
        out.extend_from_slice(&self.checkpoint_input_bits.to_le_bytes());
        out.push(buffered_value);
        out.push(bfinal_block_type);
        out.extend_from_slice(&(uncompressed_remaining as u16).to_le_bytes());
        out.extend_from_slice(&lit_codes);
        out.extend_from_slice(&dist_codes);
        out.extend_from_slice(&output_bytes_written.to_le_bytes());
        out.extend_from_slice(&bytes_unread.to_le_bytes());
        debug_assert_eq!(out.len(), CHECKPOINT_HEADER_SIZE);
        out.extend_from_slice(window_a);
        out.extend_from_slice(window_b);
        let checksum = fletcher32_checksum(&out);
        out.extend_from_slice(&checksum.to_le_bytes());

        Some((
            out,
            CheckpointStreamPositions {
                input_bytes_to_skip: self.checkpoint_input_bits.div_ceil(8),
                output_bytes_already_returned: output_bytes_written - bytes_unread as u64,
            },
        ))
    }

    /// Restore inflater state from a previously serialized checkpoint.
    ///
    /// Returns `None` if the data is corrupt, invalid, or from an incompatible
    /// library version. On success, the inflater's internal state is overwritten
    /// and the caller must seek input/output streams according to the returned
    /// `CheckpointStreamPositions`.
    ///
    /// If the inflater has an output byte limit from
    /// [`with_uncompressed_size()`](Self::with_uncompressed_size), that limit is
    /// retained and checkpoints exceeding it will not be restored.
    #[cfg_attr(docsrs, doc(cfg(feature = "checkpoint")))]
    #[must_use]
    pub fn restore_from_checkpoint(
        &mut self,
        checkpoint_data: &[u8],
    ) -> Option<CheckpointStreamPositions> {
        if checkpoint_data.len() < CHECKPOINT_HEADER_SIZE + 4 {
            return None;
        }
        let (data, checksum_bytes) = checkpoint_data.split_at(checkpoint_data.len() - 4);
        let stored_checksum = u32::from_le_bytes(checksum_bytes.try_into().ok()?);
        if fletcher32_checksum(data) != stored_checksum {
            return None;
        }

        let mut cursor = data;
        let mut read = |n: usize| -> Option<&[u8]> {
            if cursor.len() < n {
                return None;
            }
            let (head, tail) = cursor.split_at(n);
            cursor = tail;
            Some(head)
        };

        let version: u16 = u16::from_le_bytes(read(2)?.try_into().ok()?);
        if version != 0x1001 {
            return None;
        }
        let input_bits: u64 = u64::from_le_bytes(read(8)?.try_into().ok()?);
        let buffered_value: u8 = read(1)?[0];
        let bfinal_block_type: u8 = read(1)?[0];
        let remaining_uncompressed: u16 = u16::from_le_bytes(read(2)?.try_into().ok()?);
        let lit_codes: &[u8] = read(HuffmanTree::MAX_LITERAL_TREE_ELEMENTS)?;
        let dist_codes: &[u8] = read(HuffmanTree::MAX_DIST_TREE_ELEMENTS)?;
        let output_bytes_written: u64 = u64::from_le_bytes(read(8)?.try_into().ok()?);
        let output_bytes_unread: u32 = u32::from_le_bytes(read(4)?.try_into().ok()?);
        let window_data: &[u8] = cursor;

        let num_buffered_bits = (8 - (input_bits & 7)) as i32 & 7;
        let bits = BitsBuffer::from_bits(buffered_value as u32, num_buffered_bits);

        let expected_window_len = (output_bytes_written.min(TABLE_LOOKUP_DISTANCE_MAX as u64)
            as u32)
            .max(output_bytes_unread) as usize;
        if window_data.len() != expected_window_len
            || window_data.len() > crate::output_window::WINDOW_SIZE
        {
            return None;
        }

        let output_already_returned = output_bytes_written - output_bytes_unread as u64;
        if self.uncompressed_size != usize::MAX
            && output_already_returned > self.uncompressed_size as u64
        {
            return None;
        }

        let bfinal = (bfinal_block_type & 128) != 0;
        let block_type = BlockType::from_int((bfinal_block_type % 128).into())?;

        let mut lit_tree = HuffmanTree::invalid();
        let mut dist_tree = HuffmanTree::invalid();
        if block_type == BlockType::Dynamic {
            if lit_codes.iter().any(|x| *x > 16) || dist_codes.iter().any(|x| *x > 16) {
                return None;
            }
            lit_tree.new_in_place(lit_codes).ok()?;
            dist_tree.new_in_place(dist_codes).ok()?;
        } else if block_type == BlockType::Uncompressed
            && remaining_uncompressed > 0
            && bits.bits_in_buffer != 0
        {
            return None;
        }

        // All validation passed - modify self
        self.bits = bits;
        self.checkpoint_input_bits = input_bits;
        self.checkpoint_bit_buffer = buffered_value;
        self.total_output_consumed = output_bytes_written - output_bytes_unread as u64;
        self.total_input_loaded = input_bits.div_ceil(8);

        self.output
            .restore_from_checkpoint(window_data, output_bytes_unread as usize);

        self.checkpoint_bfinal_block_type = bfinal_block_type;
        match block_type {
            BlockType::Uncompressed => {
                self.bfinal = bfinal;
                self.block_type = BlockType::Uncompressed;
                self.block_length = remaining_uncompressed as usize;
                if remaining_uncompressed > 0 {
                    self.state = InflaterState::DecodingUncompressed;
                } else if !bfinal {
                    self.state = InflaterState::ReadingBFinal;
                } else {
                    self.state = InflaterState::Done;
                }
            }
            BlockType::Static => {
                self.bfinal = bfinal;
                self.block_type = BlockType::Static;
                self.literal_length_tree = HuffmanTree::static_literal_length_tree();
                self.distance_tree = HuffmanTree::static_distance_tree();
                self.state = InflaterState::DecodeTop;
            }
            BlockType::Dynamic => {
                self.bfinal = bfinal;
                self.block_type = BlockType::Dynamic;
                self.literal_length_tree = lit_tree;
                self.distance_tree = dist_tree;
                self.state = InflaterState::DecodeTop;
            }
        }

        Some(CheckpointStreamPositions {
            input_bytes_to_skip: input_bits.div_ceil(8),
            output_bytes_already_returned: output_bytes_written - output_bytes_unread as u64,
        })
    }
}

/// Input and output stream positions corresponding to an inflater checkpoint.
#[derive(Debug, PartialEq, Eq)]
pub struct CheckpointStreamPositions {
    /// Count of input bytes already consumed before checkpoint.
    pub input_bytes_to_skip: u64,
    /// Count of output bytes already returned before checkpoint.
    pub output_bytes_already_returned: u64,
}
