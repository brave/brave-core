use byteorder::WriteBytesExt;
use std::io;

pub struct RangeEncoder<'a, W>
where
    W: 'a + io::Write,
{
    stream: &'a mut W,
    range: u32,
    low: u64,
    cache: u8,
    cachesz: u32,
}

impl<'a, W> RangeEncoder<'a, W>
where
    W: io::Write,
{
    #[allow(clippy::let_and_return)]
    pub fn new(stream: &'a mut W) -> Self {
        let enc = Self {
            stream,
            range: 0xFFFF_FFFF,
            low: 0,
            cache: 0,
            cachesz: 1,
        };
        lzma_debug!("0 {{ range: {:08x}, low: {:010x} }}", enc.range, enc.low);
        enc
    }

    fn write_low(&mut self) -> io::Result<()> {
        if self.low < 0xFF00_0000 || self.low > 0xFFFF_FFFF {
            let mut tmp = self.cache;
            loop {
                let byte = tmp.wrapping_add((self.low >> 32) as u8);
                self.stream.write_u8(byte)?;
                lzma_debug!("> byte: {:02x}", byte);
                tmp = 0xFF;
                self.cachesz -= 1;
                if self.cachesz == 0 {
                    break;
                }
            }
            self.cache = (self.low >> 24) as u8;
        }

        self.cachesz += 1;
        self.low = (self.low << 8) & 0xFFFF_FFFF;
        Ok(())
    }

    pub fn finish(&mut self) -> io::Result<()> {
        for _ in 0..5 {
            self.write_low()?;

            lzma_debug!("$ {{ range: {:08x}, low: {:010x} }}", self.range, self.low);
        }
        Ok(())
    }

    fn normalize(&mut self) -> io::Result<()> {
        while self.range < 0x0100_0000 {
            lzma_debug!(
                "+ {{ range: {:08x}, low: {:010x}, cache: {:02x}, {} }}",
                self.range,
                self.low,
                self.cache,
                self.cachesz
            );
            self.range <<= 8;
            self.write_low()?;
            lzma_debug!(
                "* {{ range: {:08x}, low: {:010x}, cache: {:02x}, {} }}",
                self.range,
                self.low,
                self.cache,
                self.cachesz
            );
        }
        lzma_trace!("  {{ range: {:08x}, low: {:010x} }}", self.range, self.low);
        Ok(())
    }

    pub fn encode_bit(&mut self, prob: &mut u16, bit: bool) -> io::Result<()> {
        let bound: u32 = (self.range >> 11) * (*prob as u32);
        lzma_trace!(
            "  bound: {:08x}, prob: {:04x}, bit: {}",
            bound,
            prob,
            bit as u8
        );

        if bit {
            *prob -= *prob >> 5;
            self.low += bound as u64;
            self.range -= bound;
        } else {
            *prob += (0x800_u16 - *prob) >> 5;
            self.range = bound;
        }

        self.normalize()
    }

    #[cfg(test)]
    fn encode_bit_tree(
        &mut self,
        num_bits: usize,
        probs: &mut [u16],
        value: u32,
    ) -> io::Result<()> {
        debug_assert!(value.leading_zeros() as usize + num_bits >= 32);
        let mut tmp: usize = 1;
        for i in 0..num_bits {
            let bit = ((value >> (num_bits - i - 1)) & 1) != 0;
            self.encode_bit(&mut probs[tmp], bit)?;
            tmp = (tmp << 1) ^ (bit as usize);
        }
        Ok(())
    }

    #[cfg(test)]
    pub fn encode_reverse_bit_tree(
        &mut self,
        num_bits: usize,
        probs: &mut [u16],
        offset: usize,
        mut value: u32,
    ) -> io::Result<()> {
        debug_assert!(value.leading_zeros() as usize + num_bits >= 32);
        let mut tmp: usize = 1;
        for _ in 0..num_bits {
            let bit = (value & 1) != 0;
            value >>= 1;
            self.encode_bit(&mut probs[offset + tmp], bit)?;
            tmp = (tmp << 1) ^ (bit as usize);
        }
        Ok(())
    }
}

// TODO: parametrize by constant and use [u16; 1 << num_bits] as soon as Rust supports this
#[cfg(test)]
#[derive(Clone)]
pub struct BitTree {
    num_bits: usize,
    probs: Vec<u16>,
}

#[cfg(test)]
impl BitTree {
    pub fn new(num_bits: usize) -> Self {
        BitTree {
            num_bits,
            probs: vec![0x400; 1 << num_bits],
        }
    }

    pub fn encode<W: io::Write>(
        &mut self,
        rangecoder: &mut RangeEncoder<W>,
        value: u32,
    ) -> io::Result<()> {
        rangecoder.encode_bit_tree(self.num_bits, self.probs.as_mut_slice(), value)
    }

    pub fn encode_reverse<W: io::Write>(
        &mut self,
        rangecoder: &mut RangeEncoder<W>,
        value: u32,
    ) -> io::Result<()> {
        rangecoder.encode_reverse_bit_tree(self.num_bits, self.probs.as_mut_slice(), 0, value)
    }
}

#[cfg(test)]
pub struct LenEncoder {
    choice: u16,
    choice2: u16,
    low_coder: Vec<BitTree>,
    mid_coder: Vec<BitTree>,
    high_coder: BitTree,
}

#[cfg(test)]
impl LenEncoder {
    pub fn new() -> Self {
        LenEncoder {
            choice: 0x400,
            choice2: 0x400,
            low_coder: vec![BitTree::new(3); 16],
            mid_coder: vec![BitTree::new(3); 16],
            high_coder: BitTree::new(8),
        }
    }

    pub fn encode<W: io::Write>(
        &mut self,
        rangecoder: &mut RangeEncoder<W>,
        pos_state: usize,
        value: u32,
    ) -> io::Result<()> {
        let is_low: bool = value < 8;
        rangecoder.encode_bit(&mut self.choice, !is_low)?;
        if is_low {
            return self.low_coder[pos_state].encode(rangecoder, value);
        }

        let is_middle: bool = value < 16;
        rangecoder.encode_bit(&mut self.choice2, !is_middle)?;
        if is_middle {
            return self.mid_coder[pos_state].encode(rangecoder, value - 8);
        }

        self.high_coder.encode(rangecoder, value - 16)
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::decode::rangecoder::{LenDecoder, RangeDecoder};
    use crate::{decode, encode};
    use std::io::BufReader;

    fn encode_decode(prob_init: u16, bits: &[bool]) {
        let mut buf: Vec<u8> = Vec::new();

        let mut encoder = RangeEncoder::new(&mut buf);
        let mut prob = prob_init;
        for &b in bits {
            encoder.encode_bit(&mut prob, b).unwrap();
        }
        encoder.finish().unwrap();

        let mut bufread = BufReader::new(buf.as_slice());
        let mut decoder = RangeDecoder::new(&mut bufread).unwrap();
        let mut prob = prob_init;
        for &b in bits {
            assert_eq!(decoder.decode_bit(&mut prob, true).unwrap(), b);
        }
        assert!(decoder.is_finished_ok().unwrap());
    }

    #[test]
    fn test_encode_decode_zeros() {
        encode_decode(0x400, &[false; 10000]);
    }

    #[test]
    fn test_encode_decode_ones() {
        encode_decode(0x400, &[true; 10000]);
    }

    fn encode_decode_bittree(num_bits: usize, values: &[u32]) {
        let mut buf: Vec<u8> = Vec::new();

        let mut encoder = RangeEncoder::new(&mut buf);
        let mut tree = encode::rangecoder::BitTree::new(num_bits);
        for &v in values {
            tree.encode(&mut encoder, v).unwrap();
        }
        encoder.finish().unwrap();

        let mut bufread = BufReader::new(buf.as_slice());
        let mut decoder = RangeDecoder::new(&mut bufread).unwrap();
        let mut tree = decode::rangecoder::BitTree::new(num_bits);
        for &v in values {
            assert_eq!(tree.parse(&mut decoder, true).unwrap(), v);
        }
        assert!(decoder.is_finished_ok().unwrap());
    }

    #[test]
    fn test_encode_decode_bittree_zeros() {
        for num_bits in 0..16 {
            encode_decode_bittree(num_bits, &[0; 10000]);
        }
    }

    #[test]
    fn test_encode_decode_bittree_ones() {
        for num_bits in 0..16 {
            encode_decode_bittree(num_bits, &[(1 << num_bits) - 1; 10000]);
        }
    }

    #[test]
    fn test_encode_decode_bittree_all() {
        for num_bits in 0..16 {
            let max = 1 << num_bits;
            let values: Vec<u32> = (0..max).collect();
            encode_decode_bittree(num_bits, &values);
        }
    }

    fn encode_decode_reverse_bittree(num_bits: usize, values: &[u32]) {
        let mut buf: Vec<u8> = Vec::new();

        let mut encoder = RangeEncoder::new(&mut buf);
        let mut tree = encode::rangecoder::BitTree::new(num_bits);
        for &v in values {
            tree.encode_reverse(&mut encoder, v).unwrap();
        }
        encoder.finish().unwrap();

        let mut bufread = BufReader::new(buf.as_slice());
        let mut decoder = RangeDecoder::new(&mut bufread).unwrap();
        let mut tree = decode::rangecoder::BitTree::new(num_bits);
        for &v in values {
            assert_eq!(tree.parse_reverse(&mut decoder, true).unwrap(), v);
        }
        assert!(decoder.is_finished_ok().unwrap());
    }

    #[test]
    fn test_encode_decode_reverse_bittree_zeros() {
        for num_bits in 0..16 {
            encode_decode_reverse_bittree(num_bits, &[0; 10000]);
        }
    }

    #[test]
    fn test_encode_decode_reverse_bittree_ones() {
        for num_bits in 0..16 {
            encode_decode_reverse_bittree(num_bits, &[(1 << num_bits) - 1; 10000]);
        }
    }

    #[test]
    fn test_encode_decode_reverse_bittree_all() {
        for num_bits in 0..16 {
            let max = 1 << num_bits;
            let values: Vec<u32> = (0..max).collect();
            encode_decode_reverse_bittree(num_bits, &values);
        }
    }

    fn encode_decode_length(pos_state: usize, values: &[u32]) {
        let mut buf: Vec<u8> = Vec::new();

        let mut encoder = RangeEncoder::new(&mut buf);
        let mut len_encoder = LenEncoder::new();
        for &v in values {
            len_encoder.encode(&mut encoder, pos_state, v).unwrap();
        }
        encoder.finish().unwrap();

        let mut bufread = BufReader::new(buf.as_slice());
        let mut decoder = RangeDecoder::new(&mut bufread).unwrap();
        let mut len_decoder = LenDecoder::new();
        for &v in values {
            assert_eq!(
                len_decoder.decode(&mut decoder, pos_state, true).unwrap(),
                v as usize
            );
        }
        assert!(decoder.is_finished_ok().unwrap());
    }

    #[test]
    fn test_encode_decode_length_zeros() {
        for pos_state in 0..16 {
            encode_decode_length(pos_state, &[0; 10000]);
        }
    }

    #[test]
    fn test_encode_decode_length_all() {
        for pos_state in 0..16 {
            let max = (1 << 8) + 16;
            let values: Vec<u32> = (0..max).collect();
            encode_decode_length(pos_state, &values);
        }
    }
}
