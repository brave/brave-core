use crate::buffer::Buffer;
use crate::huffman_tree::HuffmanTree;
use crate::input_buffer::{BitsBuffer, InputBuffer};
use crate::output_window::OutputWindow;
use crate::{array_copy, array_copy1, BlockType, InflateResult, InflaterState, InternalErr};
use std::cmp::min;
use std::mem::MaybeUninit;

// Extra bits for length code 257 - 285.
static EXTRA_LENGTH_BITS: [u8; 29] = [
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 16,
];

// The base length for length code 257 - 285.
// The formula to get the real length for a length code is lengthBase[code - 257] + (value stored in extraBits)
static LENGTH_BASE: [u8; 29] = [
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131,
    163, 195, 227, 3,
];

// The base distance for distance code 0 - 31
// The real distance for a distance code is  distanceBasePosition[code] + (value stored in extraBits)
static DISTANCE_BASE_POSITION: [u16; 32] = [
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537,
    2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 32769, 49153,
];

// code lengths for code length alphabet is stored in following order
static CODE_ORDER: [u8; 19] = [
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
];

static STATIC_DISTANCE_TREE_TABLE: [u8; 32] = [
    0x00, 0x10, 0x08, 0x18, 0x04, 0x14, 0x0c, 0x1c, 0x02, 0x12, 0x0a, 0x1a, 0x06, 0x16, 0x0e, 0x1e,
    0x01, 0x11, 0x09, 0x19, 0x05, 0x15, 0x0d, 0x1d, 0x03, 0x13, 0x0b, 0x1b, 0x07, 0x17, 0x0f, 0x1f,
];

// source: https://github.com/dotnet/runtime/blob/82dac28143be0740d795f434db9b70f61b3b7a04/src/libraries/System.IO.Compression/src/System/IO/Compression/DeflateManaged/OutputWindow.cs#L17
// Note: the upstream comment says "65536 length as well as up to a 65538 distance", but actually
//       it looks limit is 65538 length with 65538 distance.
//       There is no mention for length / distance limit in [APPNOTE.TXT] so it's hard to know
//       which is correct, but 65538 length with 65538 distance is much reasonable since
//       original deflate have (2^8-1)+3 length with 2^15 distance, and
//       65538 is (2^16-1)+3 and 65536 is 2^15.
// [APPNOTE.TXT]: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
const TABLE_LOOKUP_LENGTH_MAX: usize = 65538;
pub(crate) const TABLE_LOOKUP_DISTANCE_MAX: usize = 65536;

/// The streaming Inflater for deflate64
///
/// This struct has big buffer so It's not recommended to move this struct.
#[derive(Debug)]
pub struct InflaterManaged {
    output: OutputWindow,
    bits: BitsBuffer,
    literal_length_tree: HuffmanTree,
    distance_tree: HuffmanTree,

    state: InflaterState,
    bfinal: bool,
    block_type: BlockType,

    // uncompressed block
    block_length_buffer: [u8; 4],
    block_length: usize,

    // compressed block
    length: usize,
    distance_code: u16,
    extra_bits: i32,

    loop_counter: u32,
    literal_length_code_count: u32,
    distance_code_count: u32,
    code_length_code_count: u32,
    code_array_size: u32,
    length_code: u16,

    code_list: [u8; HuffmanTree::MAX_LITERAL_TREE_ELEMENTS + HuffmanTree::MAX_DIST_TREE_ELEMENTS], // temporary array to store the code length for literal/Length and distance
    code_length_tree_code_length: [u8; HuffmanTree::NUMBER_OF_CODE_LENGTH_TREE_ELEMENTS],
    deflate64: bool,
    code_length_tree: HuffmanTree,
    uncompressed_size: usize,

    // Cumulative counters updated once per inflate call
    total_input_loaded: u64, // total bytes loaded into bit reader, only updated after decode()
    total_output_consumed: u64, // total bytes returned to caller (also used for uncompressed_size limit)

    // Lightweight checkpoint: updated after every write to output window
    #[cfg(feature = "checkpoint")]
    checkpoint_input_bits: u64, // exact input bit position of checkpoint
    #[cfg(feature = "checkpoint")]
    checkpoint_bit_buffer: u8, // low byte of input bit_buffer (future bits)
    #[cfg(feature = "checkpoint")]
    checkpoint_bfinal_block_type: u8, // (bfinal << 7) | block_type
}

impl InflaterManaged {
    /// Initializes Inflater
    #[allow(clippy::new_without_default)]
    #[inline]
    pub fn new() -> Self {
        Self::with_uncompressed_size(usize::MAX)
    }

    /// Initializes Inflater with expected uncompressed size.
    pub fn with_uncompressed_size(uncompressed_size: usize) -> Self {
        Self {
            output: OutputWindow::new(),
            bits: BitsBuffer::new(),

            literal_length_tree: HuffmanTree::invalid(),
            code_list: [0u8; HuffmanTree::MAX_LITERAL_TREE_ELEMENTS
                + HuffmanTree::MAX_DIST_TREE_ELEMENTS],
            code_length_tree_code_length: [0u8; HuffmanTree::NUMBER_OF_CODE_LENGTH_TREE_ELEMENTS],
            deflate64: true,
            code_length_tree: HuffmanTree::invalid(),
            uncompressed_size,
            state: InflaterState::ReadingBFinal, // start by reading BFinal bit
            bfinal: false,
            block_type: BlockType::Uncompressed,
            block_length_buffer: [0u8; 4],
            block_length: 0,
            length: 0,
            distance_code: 0,
            extra_bits: 0,
            loop_counter: 0,
            literal_length_code_count: 0,
            distance_code_count: 0,
            code_length_code_count: 0,
            code_array_size: 0,
            distance_tree: HuffmanTree::invalid(),
            length_code: 0,
            total_input_loaded: 0,
            total_output_consumed: 0,
            #[cfg(feature = "checkpoint")]
            checkpoint_input_bits: 0,
            #[cfg(feature = "checkpoint")]
            checkpoint_bit_buffer: 0,
            #[cfg(feature = "checkpoint")]
            checkpoint_bfinal_block_type: 0,
        }
    }

    /// Returns true if decompression finished and no more output is available
    ///
    /// This also returns true if this inflater is in error state
    pub fn finished(&self) -> bool {
        (self.state == InflaterState::Done && self.available_output() == 0)
            || self.state == InflaterState::DataErrored
    }

    /// Returns true if decompression finished, but may still have output available in buffer
    ///
    /// This also returns true if this inflater is in error state
    pub fn input_finished(&self) -> bool {
        self.state == InflaterState::Done || self.state == InflaterState::DataErrored
    }

    /// Returns true if this inflater is in error state
    pub fn errored(&self) -> bool {
        self.state == InflaterState::DataErrored
    }

    /// The count of bytes currently inflater has in internal output buffer
    #[allow(dead_code)]
    pub fn available_output(&self) -> usize {
        self.output.available_bytes()
    }

    /// Try to decompress from `input` to `output`.
    ///
    /// This will decompress data until `output` is full, `input` is empty,
    /// the end if the deflate64 stream is hit, or there is error data in the deflate64 stream.
    pub fn inflate(&mut self, input: &[u8], output: &mut [u8]) -> InflateResult {
        self.inflate_internal(input, Buffer::Init(output))
    }

    /// Same as [`Self::inflate`] but accepts uninitialized buffer
    pub fn inflate_uninit(
        &mut self,
        input: &[u8],
        output: &mut [MaybeUninit<u8>],
    ) -> InflateResult {
        self.inflate_internal(input, Buffer::Uninit(output))
    }

    fn inflate_internal(&mut self, input: &[u8], mut output: Buffer<'_>) -> InflateResult {
        // copy bytes from output to outputbytes if we have available bytes
        // if buffer is not filled up. keep decoding until no input are available
        // if decodeBlock returns false. Throw an exception.
        let mut result = InflateResult::new();
        let mut input = InputBuffer::new(self.bits, input);
        while 'while_loop: {
            let mut copied = 0;
            if self.uncompressed_size == usize::MAX {
                copied = self.output.copy_to(output.reborrow());
            } else if (self.uncompressed_size as u64) > self.total_output_consumed {
                let remaining =
                    (self.uncompressed_size as u64 - self.total_output_consumed) as usize;
                let len = min(output.len(), remaining);
                output = output.index_mut(..len);
                copied = self.output.copy_to(output.reborrow());
            } else {
                self.state = InflaterState::Done;
                self.output.clear_bytes_used();
            }
            if copied > 0 {
                output = output.index_mut(copied..);
                result.bytes_written += copied;
                self.total_output_consumed += copied as u64;
            }

            if output.is_empty() {
                // filled in the bytes buffer
                break 'while_loop false;
            }
            // decode will return false when more input is needed
            if self.errored() {
                result.data_error = true;
                break 'while_loop false;
            } else if self.input_finished() {
                break 'while_loop false;
            }
            match self.decode(&mut input) {
                Ok(()) => true,
                Err(InternalErr::DataNeeded) => false,
                Err(InternalErr::DataError) => {
                    self.state = InflaterState::DataErrored;
                    result.data_error = true;
                    false
                }
            }
        } {}

        self.bits = input.bits;
        self.total_input_loaded += input.read_bytes as u64;
        result.bytes_consumed = input.read_bytes;
        result
    }

    fn decode(&mut self, input: &mut InputBuffer<'_>) -> Result<(), InternalErr> {
        let mut eob = false;
        let result;

        if self.errored() {
            return Err(InternalErr::DataError);
        } else if self.input_finished() {
            return Ok(());
        }

        if self.state == InflaterState::ReadingBFinal {
            // reading bfinal bit
            // Need 1 bit
            self.bfinal = input.get_bits(1)? != 0;
            self.state = InflaterState::ReadingBType;
        }

        if self.state == InflaterState::ReadingBType {
            // Need 2 bits
            self.state = InflaterState::ReadingBType;
            let bits = input.get_bits(2)?;

            self.block_type = BlockType::from_int(bits).ok_or(InternalErr::DataError)?;
            match self.block_type {
                BlockType::Dynamic => {
                    self.state = InflaterState::ReadingNumLitCodes;
                }
                BlockType::Static => {
                    self.literal_length_tree = HuffmanTree::static_literal_length_tree();
                    self.distance_tree = HuffmanTree::static_distance_tree();
                    self.state = InflaterState::DecodeTop;
                }
                BlockType::Uncompressed => {
                    self.state = InflaterState::UncompressedAligning;
                }
            }
        }

        if self.block_type == BlockType::Dynamic {
            if self.state < InflaterState::DecodeTop {
                // we are reading the header
                result = self.decode_dynamic_block_header(input);
            } else {
                result = self.decode_block(input, &mut eob); // this can returns true when output is full
            }
        } else if self.block_type == BlockType::Static {
            result = self.decode_block(input, &mut eob);
        } else if self.block_type == BlockType::Uncompressed {
            result = self.decode_uncompressed_block(input, &mut eob);
        } else {
            result = Err(InternalErr::DataError); // UnknownBlockType
        }

        //
        // If we reached the end of the block and the block we were decoding had
        // bfinal=1 (final block)
        //
        if eob && self.bfinal {
            self.state = InflaterState::Done;
        }
        result
    }

    fn decode_uncompressed_block(
        &mut self,
        input: &mut InputBuffer<'_>,
        end_of_block: &mut bool,
    ) -> Result<(), InternalErr> {
        *end_of_block = false;
        loop {
            match self.state {
                InflaterState::UncompressedAligning => {
                    input.skip_to_byte_boundary();
                    self.state = InflaterState::UncompressedByte1;
                    continue; //goto case InflaterState.UncompressedByte1;
                }
                InflaterState::UncompressedByte1
                | InflaterState::UncompressedByte2
                | InflaterState::UncompressedByte3
                | InflaterState::UncompressedByte4 => {
                    self.block_length_buffer
                        [(self.state - InflaterState::UncompressedByte1) as usize] =
                        input.get_bits(8)? as u8;
                    if self.state == InflaterState::UncompressedByte4 {
                        self.block_length = self.block_length_buffer[0] as usize
                            + (self.block_length_buffer[1] as usize) * 256;
                        let block_length_complement: i32 = self.block_length_buffer[2] as i32
                            + (self.block_length_buffer[3] as i32) * 256;

                        // make sure complement matches
                        if self.block_length as u16 != !block_length_complement as u16 {
                            return Err(InternalErr::DataError); // InvalidBlockLength
                        }
                    }

                    self.state = match self.state {
                        InflaterState::UncompressedByte1 => InflaterState::UncompressedByte2,
                        InflaterState::UncompressedByte2 => InflaterState::UncompressedByte3,
                        InflaterState::UncompressedByte3 => InflaterState::UncompressedByte4,
                        InflaterState::UncompressedByte4 => InflaterState::DecodingUncompressed,
                        _ => unreachable!(),
                    };
                }
                InflaterState::DecodingUncompressed => {
                    // Directly copy bytes from input to output.
                    let bytes_copied = self.output.copy_from(input, self.block_length);
                    self.block_length -= bytes_copied;

                    if self.block_length == 0 {
                        // Done with this block, need to re-init bit buffer for next block
                        self.state = InflaterState::ReadingBFinal;
                        *end_of_block = true;
                        self.update_checkpoint_after_write_or_eob(input, true);
                        return Ok(());
                    }

                    self.update_checkpoint_after_write_or_eob(input, false);

                    // We can fail to copy all bytes for two reasons:
                    //    Running out of Input
                    //    running out of free space in output window
                    if self.output.free_bytes() == 0 {
                        return Ok(());
                    }

                    return Err(InternalErr::DataNeeded);
                }
                _ => {
                    panic!("UnknownState");
                }
            }
        }
    }

    fn decode_block(
        &mut self,
        input: &mut InputBuffer<'_>,
        end_of_block_code_seen: &mut bool,
    ) -> Result<(), InternalErr> {
        *end_of_block_code_seen = false;

        if self.state == InflaterState::DecodeTop {
            // Tight inner loop for decoding and processing deflate symbols when we know
            // that there is both enough input available and also sufficient output space.
            // State machine variables are not modified and self.state stays as DecodeTop.
            match self.decode_block_fast_inner_loop(input) {
                Ok((_, true)) => {
                    // End of block reached
                    *end_of_block_code_seen = true;
                    self.state = InflaterState::ReadingBFinal;
                    self.update_checkpoint_after_write_or_eob(input, true);
                    return Ok(());
                }
                Ok((0, false)) => {
                    // No fast progress, fall through to slower but comprehensive
                    // state machine implementation which can load partial input.
                }
                Ok((_, false)) => {
                    // Some fast progress was made. Return so that output can be
                    // consumed by the caller and/or more input can be provided.
                    self.update_checkpoint_after_write_or_eob(input, false);
                    return Ok(());
                }
                Err(InternalErr::DataError) => {
                    return Err(InternalErr::DataError);
                }
                Err(InternalErr::DataNeeded) => {
                    unreachable!("fast inner loop never returns DataNeeded")
                }
            }
        }

        // State machine path
        let mut free_bytes = self.output.free_bytes();
        while free_bytes >= TABLE_LOOKUP_LENGTH_MAX {
            // With Deflate64 we can have up to a 64kb length, so we ensure at least that much space is available
            // in the OutputWindow to avoid overwriting previous unflushed output data.

            let mut symbol;
            match self.state {
                InflaterState::DecodeTop => {
                    // decode an element from the literal tree

                    // TODO: optimize this!!!
                    symbol = self.literal_length_tree.get_next_symbol(input)?;

                    #[allow(clippy::comparison_chain)]
                    if symbol < 256 {
                        // literal
                        self.output.write(symbol as u8);
                        free_bytes -= 1;
                        self.update_checkpoint_after_write_or_eob(input, false);
                    } else if symbol == 256 {
                        // end of block
                        *end_of_block_code_seen = true;
                        // Reset state
                        self.state = InflaterState::ReadingBFinal;
                        self.update_checkpoint_after_write_or_eob(input, true);
                        return Ok(());
                    } else {
                        // length/distance pair
                        symbol -= 257; // length code started at 257
                        if symbol < 8 {
                            symbol += 3; // match length = 3,4,5,6,7,8,9,10
                            self.extra_bits = 0;
                        } else if !self.deflate64 && symbol == 28 {
                            // extra bits for code 285 is 0
                            symbol = 258; // code 285 means length 258
                            self.extra_bits = 0;
                        } else {
                            if symbol as usize >= EXTRA_LENGTH_BITS.len() {
                                return Err(InternalErr::DataError); // GenericInvalidData
                            }
                            self.extra_bits = EXTRA_LENGTH_BITS[symbol as usize] as i32;
                            assert_ne!(self.extra_bits, 0, "We handle other cases separately!");
                        }
                        self.length = symbol as usize;

                        self.state = InflaterState::HaveInitialLength;
                        continue; //goto case InflaterState::HaveInitialLength;
                    }
                }
                InflaterState::HaveInitialLength => {
                    if self.extra_bits > 0 {
                        self.state = InflaterState::HaveInitialLength;
                        let bits = input.get_bits(self.extra_bits)?;

                        if self.length >= LENGTH_BASE.len() {
                            return Err(InternalErr::DataError); // GenericInvalidData
                        }
                        self.length = LENGTH_BASE[self.length] as usize + bits as usize;
                    }
                    self.state = InflaterState::HaveFullLength;
                    continue; // goto case InflaterState::HaveFullLength;
                }
                InflaterState::HaveFullLength => {
                    if self.block_type == BlockType::Dynamic {
                        let bits = self.distance_tree.get_next_symbol(input)?;
                        self.distance_code = bits;
                    } else {
                        // get distance code directly for static block
                        let bits = input.get_bits(5)?;
                        self.distance_code = STATIC_DISTANCE_TREE_TABLE[bits as usize] as u16;
                    }

                    self.state = InflaterState::HaveDistCode;
                    continue; //goto case InflaterState.HaveDistCode;
                }

                InflaterState::HaveDistCode => {
                    // To avoid a table lookup we note that for distanceCode > 3,
                    // extra_bits = (distanceCode-2) >> 1
                    let offset: usize;
                    if self.distance_code > 3 {
                        self.extra_bits = ((self.distance_code - 2) >> 1) as i32;
                        let bits = input.get_bits(self.extra_bits)?;
                        offset = DISTANCE_BASE_POSITION[self.distance_code as usize] as usize
                            + bits as usize;
                    } else {
                        offset = (self.distance_code + 1) as usize;
                    }

                    if self.length > TABLE_LOOKUP_LENGTH_MAX || offset > TABLE_LOOKUP_DISTANCE_MAX {
                        return Err(InternalErr::DataError);
                    }

                    self.output.write_length_distance(self.length, offset);
                    free_bytes -= self.length;
                    self.state = InflaterState::DecodeTop;
                    self.update_checkpoint_after_write_or_eob(input, false);
                }

                _ => {
                    //Debug.Fail("check why we are here!");
                    panic!("UnknownState");
                }
            }
        }

        Ok(())
    }

    /// Fast inner loop for decoding literals and length/distance pairs. Breaks out as soon as
    /// maximum possible output cannot fit, or maximum possible input required is not available.
    /// Returns (bytes_written, end_of_block) on success or InternalErr on failure.
    fn decode_block_fast_inner_loop(
        &mut self,
        input: &mut InputBuffer<'_>,
    ) -> Result<(usize, bool), InternalErr> {
        let initial_free = self.output.free_bytes();

        loop {
            // Exit fast path if low on output space or input bits.
            // Maximum input consumed per iteration is 64 bits, or 8 bytes:
            //  16 bits for initial symbol value >= 257, indicating match length
            //  16 bits for match length "extra bits"
            //  16 bits for distance symbol
            //  16 bits for distance "extra bits"
            if self.output.free_bytes() < TABLE_LOOKUP_LENGTH_MAX || input.available_bytes() < 8 {
                return Ok((initial_free - self.output.free_bytes(), false));
            }

            let symbol = self
                .literal_length_tree
                .get_next_symbol_assume_input(input)?;
            match symbol {
                0..=255 => {
                    // Literal byte
                    self.output.write(symbol as u8);
                }
                256 => {
                    // End of block
                    return Ok((initial_free - self.output.free_bytes(), true));
                }
                257..=285 => {
                    // Length/distance pair
                    let length_index = (symbol - 257) as usize;
                    let length = if length_index < 8 {
                        length_index + 3
                    } else {
                        let extra_bits = EXTRA_LENGTH_BITS[length_index] as i32;
                        let bits = input.get_bits_assume_input(extra_bits);
                        LENGTH_BASE[length_index] as usize + bits as usize
                    };

                    let distance_code = if self.block_type == BlockType::Dynamic {
                        self.distance_tree.get_next_symbol_assume_input(input)? as usize
                    } else {
                        STATIC_DISTANCE_TREE_TABLE[input.get_bits_assume_input(5) as usize] as usize
                    };

                    let offset = if distance_code <= 3 {
                        distance_code + 1
                    } else {
                        let extra_bits = ((distance_code - 2) >> 1) as i32;
                        let bits = input.get_bits_assume_input(extra_bits);
                        *DISTANCE_BASE_POSITION
                            .get(distance_code)
                            .ok_or(InternalErr::DataError)? as usize
                            + bits as usize
                    };

                    if length > TABLE_LOOKUP_LENGTH_MAX || offset > TABLE_LOOKUP_DISTANCE_MAX {
                        return Err(InternalErr::DataError);
                    }
                    self.output.write_length_distance(length, offset);
                }
                _ => {
                    // Symbol out of range
                    return Err(InternalErr::DataError);
                }
            }
        }
    }

    // Format of the dynamic block header:
    //      5 Bits: HLIT, # of Literal/Length codes - 257 (257 - 286)
    //      5 Bits: HDIST, # of Distance codes - 1        (1 - 32)
    //      4 Bits: HCLEN, # of Code Length codes - 4     (4 - 19)
    //
    //      (HCLEN + 4) x 3 bits: code lengths for the code length
    //          alphabet given just above, in the order: 16, 17, 18,
    //          0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    //
    //          These code lengths are interpreted as 3-bit integers
    //          (0-7); as above, a code length of 0 means the
    //          corresponding symbol (literal/length or distance code
    //          length) is not used.
    //
    //      HLIT + 257 code lengths for the literal/length alphabet,
    //          encoded using the code length Huffman code
    //
    //       HDIST + 1 code lengths for the distance alphabet,
    //          encoded using the code length Huffman code
    //
    // The code length repeat codes can cross from HLIT + 257 to the
    // HDIST + 1 code lengths.  In other words, all code lengths form
    // a single sequence of HLIT + HDIST + 258 values.
    fn decode_dynamic_block_header(
        &mut self,
        input: &mut InputBuffer<'_>,
    ) -> Result<(), InternalErr> {
        'switch: loop {
            match self.state {
                InflaterState::ReadingNumLitCodes => {
                    let bits = input.get_bits(5)?;
                    self.literal_length_code_count = bits as u32 + 257;
                    self.state = InflaterState::ReadingNumDistCodes;
                    continue 'switch; //goto case InflaterState::ReadingNumDistCodes;
                }
                InflaterState::ReadingNumDistCodes => {
                    let bits = input.get_bits(5)?;
                    self.distance_code_count = bits as u32 + 1;
                    self.state = InflaterState::ReadingNumCodeLengthCodes;
                    continue 'switch; // goto case InflaterState::ReadingNumCodeLengthCodes;
                }
                InflaterState::ReadingNumCodeLengthCodes => {
                    let bits = input.get_bits(4)?;
                    self.code_length_code_count = bits as u32 + 4;
                    self.loop_counter = 0;
                    self.state = InflaterState::ReadingCodeLengthCodes;
                    continue 'switch; // goto case InflaterState::ReadingCodeLengthCodes;
                }
                InflaterState::ReadingCodeLengthCodes => {
                    while self.loop_counter < self.code_length_code_count {
                        let bits = input.get_bits(3)?;
                        self.code_length_tree_code_length
                            [CODE_ORDER[self.loop_counter as usize] as usize] = bits as u8;
                        self.loop_counter += 1;
                    }

                    for &code_oder in &CODE_ORDER[self.code_length_code_count as usize..] {
                        self.code_length_tree_code_length[code_oder as usize] = 0;
                    }

                    // create huffman tree for code length
                    self.code_length_tree
                        .new_in_place(&self.code_length_tree_code_length)?;
                    self.code_array_size =
                        self.literal_length_code_count + self.distance_code_count;
                    self.loop_counter = 0; // reset loop count

                    self.state = InflaterState::ReadingTreeCodesBefore;
                    continue 'switch; // goto case InflaterState::ReadingTreeCodesBefore;
                }
                InflaterState::ReadingTreeCodesBefore | InflaterState::ReadingTreeCodesAfter => {
                    while self.loop_counter < self.code_array_size {
                        if self.state == InflaterState::ReadingTreeCodesBefore {
                            self.length_code = self.code_length_tree.get_next_symbol(input)?;
                        }

                        // The alphabet for code lengths is as follows:
                        //  0 - 15: Represent code lengths of 0 - 15
                        //  16: Copy the previous code length 3 - 6 times.
                        //  The next 2 bits indicate repeat length
                        //         (0 = 3, ... , 3 = 6)
                        //      Example:  Codes 8, 16 (+2 bits 11),
                        //                16 (+2 bits 10) will expand to
                        //                12 code lengths of 8 (1 + 6 + 5)
                        //  17: Repeat a code length of 0 for 3 - 10 times.
                        //    (3 bits of length)
                        //  18: Repeat a code length of 0 for 11 - 138 times
                        //    (7 bits of length)
                        if self.length_code <= 15 {
                            self.code_list[self.loop_counter as usize] = self.length_code as u8;
                            self.loop_counter += 1;
                        } else {
                            let repeat_count: u32;
                            if self.length_code == 16 {
                                self.state = InflaterState::ReadingTreeCodesAfter;

                                if self.loop_counter == 0 {
                                    // can't have "prev code" on first code
                                    return Err(InternalErr::DataError);
                                }

                                let bits = input.get_bits(2)?;

                                let previous_code = self.code_list[self.loop_counter as usize - 1];
                                repeat_count = (bits + 3) as u32;

                                if self.loop_counter + repeat_count > self.code_array_size {
                                    //throw new InvalidDataException();
                                    return Err(InternalErr::DataError);
                                }

                                for _ in 0..repeat_count {
                                    self.code_list[self.loop_counter as usize] = previous_code;
                                    self.loop_counter += 1;
                                }
                            } else if self.length_code == 17 {
                                self.state = InflaterState::ReadingTreeCodesAfter;
                                let bits = input.get_bits(3)?;

                                repeat_count = (bits + 3) as u32;

                                if self.loop_counter + repeat_count > self.code_array_size {
                                    //throw new InvalidDataException();
                                    return Err(InternalErr::DataError);
                                }

                                for _ in 0..repeat_count {
                                    self.code_list[self.loop_counter as usize] = 0;
                                    self.loop_counter += 1;
                                }
                            } else {
                                // code == 18
                                self.state = InflaterState::ReadingTreeCodesAfter;
                                let bits = input.get_bits(7)?;

                                repeat_count = (bits + 11) as u32;

                                if self.loop_counter + repeat_count > self.code_array_size {
                                    //throw new InvalidDataException();
                                    return Err(InternalErr::DataError);
                                }

                                for _ in 0..repeat_count {
                                    self.code_list[self.loop_counter as usize] = 0;
                                    self.loop_counter += 1;
                                }
                            }
                        }
                        self.state = InflaterState::ReadingTreeCodesBefore; // we want to read the next code.
                    }
                    break 'switch;
                }
                _ => {
                    panic!("InvalidDataException: UnknownState");
                }
            }
        }

        let mut literal_tree_code_length = [0u8; HuffmanTree::MAX_LITERAL_TREE_ELEMENTS];
        let mut distance_tree_code_length = [0u8; HuffmanTree::MAX_DIST_TREE_ELEMENTS];

        // Create literal and distance tables
        array_copy(
            &self.code_list,
            &mut literal_tree_code_length,
            self.literal_length_code_count as usize,
        );
        array_copy1(
            &self.code_list,
            self.literal_length_code_count as usize,
            &mut distance_tree_code_length,
            0,
            self.distance_code_count as usize,
        );

        // Make sure there is an end-of-block code, otherwise how could we ever end?
        if literal_tree_code_length[HuffmanTree::END_OF_BLOCK_CODE] == 0 {
            return Err(InternalErr::DataError); // InvalidDataException
        }

        self.literal_length_tree
            .new_in_place(&literal_tree_code_length)?;
        self.distance_tree
            .new_in_place(&distance_tree_code_length)?;
        self.state = InflaterState::DecodeTop;
        Ok(())
    }

    #[inline(always)]
    #[allow(unused_variables)]
    fn update_checkpoint_after_write_or_eob(
        &mut self,
        input: &InputBuffer<'_>,
        end_of_block: bool,
    ) {
        #[cfg(feature = "checkpoint")]
        checkpoint::update_checkpoint(self, input, end_of_block);
    }
}

#[cfg(feature = "checkpoint")]
#[path = "inflater_checkpoint.rs"]
pub mod checkpoint;
