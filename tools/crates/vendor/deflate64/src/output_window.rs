use crate::{buffer::Buffer, input_buffer::InputBuffer};
use std::cmp::min;

// With Deflate64 we can have up to a 65536 length as well as up to a 65538 distance. We need a power-of-two
// window size that goes back at least 65538 bytes, and we can only write into it when there are at least
// 65536 "free" bytes available for the maximum possible write length. However, it is OK if the free bytes
// overlap the history window; we process length-distance match copies in the forward direction. It is fine
// to wrap around and overwrite bytes that we have already copied forward.
pub(crate) const WINDOW_SIZE: usize = 131072;
const WINDOW_MASK: usize = 131071;

/// <summary>
/// This class maintains a window for decompressed output.
/// We need to keep this because the decompressed information can be
/// a literal or a length/distance pair. For length/distance pair,
/// we need to look back in the output window and copy bytes from there.
/// We use a byte array of WINDOW_SIZE circularly.
/// </summary>
#[derive(Debug)]
pub(crate) struct OutputWindow {
    window: [u8; WINDOW_SIZE],
    end: usize,
    bytes_used: usize,
}

impl OutputWindow {
    pub fn new() -> Self {
        Self {
            window: [0; WINDOW_SIZE],
            end: 0,
            bytes_used: 0,
        }
    }

    pub(crate) fn clear_bytes_used(&mut self) {
        self.bytes_used = 0;
    }

    /// <summary>Add a byte to output window.</summary>
    #[inline(always)]
    pub fn write(&mut self, b: u8) {
        debug_assert!(
            self.bytes_used < WINDOW_SIZE,
            "Can't add byte when window is full!"
        );
        self.window[self.end] = b;
        self.end += 1;
        self.end &= WINDOW_MASK;
        self.bytes_used += 1;
    }

    #[inline(always)]
    pub fn write_length_distance(&mut self, length: usize, distance: usize) {
        debug_assert!((self.bytes_used + length) <= WINDOW_SIZE, "No Enough space");

        // move backwards distance bytes in the output stream,
        // and copy length bytes from this position to the output stream.

        // This function *could* have lots of special-case optimizations for long
        // non-overlapping copies, repeated bytes / patterns for long fills with
        // short distances, separate paths for wrapping/non-wrapping writes, etc.
        // but simpler ends up faster due to inlining and avoiding misprediction.
        self.bytes_used += length;
        let mut from = self.end.wrapping_sub(distance) & WINDOW_MASK;
        let mut to = self.end;

        for _ in 0..length {
            self.window[to] = self.window[from];
            to = (to + 1) & WINDOW_MASK;
            from = (from + 1) & WINDOW_MASK;
        }

        self.end = to;
    }

    /// <summary>
    /// Copy up to length of bytes from input directly.
    /// This is used for uncompressed block.
    /// </summary>
    pub fn copy_from(&mut self, input: &mut InputBuffer<'_>, mut length: usize) -> usize {
        length = min(
            min(length, WINDOW_SIZE - self.bytes_used),
            input.available_bytes(),
        );
        let mut copied: usize;

        // We might need wrap around to copy all bytes.
        let tail_len = WINDOW_SIZE - self.end;
        if length > tail_len {
            // copy the first part
            copied = input.copy_to(&mut self.window[self.end..][..tail_len]);
            if copied == tail_len {
                // only try to copy the second part if we have enough bytes in input
                copied += input.copy_to(&mut self.window[..length - tail_len]);
            }
        } else {
            // only one copy is needed if there is no wrap around.
            copied = input.copy_to(&mut self.window[self.end..][..length]);
        }

        self.end = (self.end + copied) & WINDOW_MASK;
        self.bytes_used += copied;
        copied
    }

    /// <summary>Free space in output window.</summary>
    pub fn free_bytes(&self) -> usize {
        WINDOW_SIZE - self.bytes_used
    }

    /// <summary>Bytes not consumed in output window.</summary>
    pub fn available_bytes(&self) -> usize {
        self.bytes_used
    }

    /// <summary>Copy the decompressed bytes to output buffer.</summary>
    pub fn copy_to(&mut self, output: Buffer<'_>) -> usize {
        let (copy_end, mut output) = if output.len() > self.bytes_used {
            // we can copy all the decompressed bytes out
            (self.end, output.index_mut(..self.bytes_used))
        } else {
            // copy length of bytes
            (
                (self
                    .end
                    .overflowing_sub(self.bytes_used)
                    .0
                    .overflowing_add(output.len())
                    .0)
                    & WINDOW_MASK,
                output,
            )
        };

        let copied = output.len();

        let mut output = if output.len() > copy_end {
            let tail_len = output.len() - copy_end;
            // this means we need to copy two parts separately
            // copy the tail_len bytes from the end of the output window
            output
                .reborrow()
                .index_mut(..tail_len)
                .copy_from_slice(&self.window[WINDOW_SIZE - tail_len..][..tail_len]);
            output.index_mut(tail_len..).index_mut(..copy_end)
        } else {
            output
        };
        output.copy_from_slice(&self.window[copy_end - output.len()..][..output.len()]);
        self.bytes_used -= copied;
        //debug_assert!(self.bytes_used >= 0, "check this function and find why we copied more bytes than we have");
        copied
    }

    #[cfg(feature = "checkpoint")]
    pub(crate) fn get_checkpoint_data(&self, total_output_written: u64) -> (&[u8], &[u8]) {
        use crate::inflater_managed::TABLE_LOOKUP_DISTANCE_MAX;
        let history_needed = min(TABLE_LOOKUP_DISTANCE_MAX, total_output_written as usize);
        let data_len = history_needed.max(self.bytes_used);
        let start = (self.end + WINDOW_SIZE - data_len) & WINDOW_MASK;
        if data_len <= WINDOW_SIZE - start {
            // one contiguous range
            (&self.window[start..start + data_len], &[])
        } else {
            // wrap around, two ranges
            (&self.window[start..], &self.window[..self.end])
        }
    }

    #[cfg(feature = "checkpoint")]
    pub(crate) fn restore_from_checkpoint(&mut self, data: &[u8], bytes_used: usize) {
        self.window[..data.len()].copy_from_slice(data);
        self.end = data.len();
        self.bytes_used = bytes_used;
    }
}
