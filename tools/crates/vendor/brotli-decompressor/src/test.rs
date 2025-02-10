#![cfg(test)]

extern crate alloc_no_stdlib as alloc;
use alloc::{AllocatedStackMemory, Allocator, SliceWrapper, SliceWrapperMut, StackAllocator, bzero};
#[cfg(feature="std")]
use std::vec::Vec;
#[cfg(feature="std")]
use std::io;

use core::ops;


pub use super::{BrotliDecompressStream, BrotliResult, BrotliState, HuffmanCode};

declare_stack_allocator_struct!(MemPool, 4096, stack);



fn oneshot(input: &mut [u8], mut output: &mut [u8]) -> (BrotliResult, usize, usize) {
  let mut available_out: usize = output.len();
  let mut stack_u8_buffer = define_allocator_memory_pool!(4096, u8, [0; 300 * 1024], stack);
  let mut stack_u32_buffer = define_allocator_memory_pool!(4096, u32, [0; 12 * 1024], stack);
  let mut stack_hc_buffer = define_allocator_memory_pool!(4096,
                                                          super::HuffmanCode,
                                                          [HuffmanCode::default(); 18 * 1024],
                                                          stack);
  let stack_u8_allocator = MemPool::<u8>::new_allocator(&mut stack_u8_buffer, bzero);
  let stack_u32_allocator = MemPool::<u32>::new_allocator(&mut stack_u32_buffer, bzero);
  let stack_hc_allocator = MemPool::<HuffmanCode>::new_allocator(&mut stack_hc_buffer, bzero);
  let mut available_in: usize = input.len();
  let mut input_offset: usize = 0;
  let mut output_offset: usize = 0;
  let mut written: usize = 0;
  let mut brotli_state =
    BrotliState::new(stack_u8_allocator, stack_u32_allocator, stack_hc_allocator);
  let result = BrotliDecompressStream(&mut available_in,
                                      &mut input_offset,
                                      &input[..],
                                      &mut available_out,
                                      &mut output_offset,
                                      &mut output,
                                      &mut written,
                                      &mut brotli_state);
  return (result, input_offset, output_offset);
}

#[test]
fn test_10x10y() {
  const BUFFER_SIZE: usize = 2048;
  let mut input: [u8; 12] = [0x1b, 0x13, 0x00, 0x00, 0xa4, 0xb0, 0xb2, 0xea, 0x81, 0x47, 0x02,
                             0x8a];
  let mut output = [0u8; BUFFER_SIZE];
  let (result, input_offset, output_offset) = oneshot(&mut input[..], &mut output[..]);
  match result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  let mut i: usize = 0;
  while i < 10 {
    assert_eq!(output[i], 'X' as u8);
    assert_eq!(output[i + 10], 'Y' as u8);
    i += 1;
  }
  assert_eq!(output_offset, 20);
  assert_eq!(input_offset, input.len());
}



#[test]
fn test_x() {
  const BUFFER_SIZE: usize = 128;
  let mut input: [u8; 5] = [0x0b, 0x00, 0x80, 0x58, 0x03];
  let mut output = [0u8; BUFFER_SIZE];
  let (result, input_offset, output_offset) = oneshot(&mut input[..], &mut output[..]);
  match result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  assert_eq!(output[0], 'X' as u8);
  assert_eq!(output_offset, 1);
  assert_eq!(input_offset, input.len());
}

#[test]
fn test_empty() {
  const BUFFER_SIZE: usize = 128;
  let mut input: [u8; 1] = [0x06];
  let mut output = [0u8; BUFFER_SIZE];
  let (result, input_offset, output_offset) = oneshot(&mut input[..], &mut output[..]);
  match result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  assert_eq!(output_offset, 0);
  assert_eq!(input_offset, input.len());
}
const QF_BUFFER_SIZE: usize = 180 * 1024;
static mut quick_fox_output: [u8; QF_BUFFER_SIZE] = [0u8; QF_BUFFER_SIZE];

#[test]
fn test_quickfox_repeated_custom() {
  let mut input: [u8; 58] =
    [0x5B, 0xFF, 0xAF, 0x02, 0xC0, 0x22, 0x79, 0x5C, 0xFB, 0x5A, 0x8C, 0x42, 0x3B, 0xF4, 0x25,
     0x55, 0x19, 0x5A, 0x92, 0x99, 0xB1, 0x35, 0xC8, 0x19, 0x9E, 0x9E, 0x0A, 0x7B, 0x4B, 0x90,
     0xB9, 0x3C, 0x98, 0xC8, 0x09, 0x40, 0xF3, 0xE6, 0xD9, 0x4D, 0xE4, 0x6D, 0x65, 0x1B, 0x27,
     0x87, 0x13, 0x5F, 0xA6, 0xE9, 0x30, 0x96, 0x7B, 0x3C, 0x15, 0xD8, 0x53, 0x1C];

  let (result, input_offset, output_offset) = oneshot(&mut input[..], &mut unsafe{&mut quick_fox_output[..]});
  match result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  assert_eq!(output_offset, 176128);
  assert_eq!(input_offset, input.len());
  const fox: [u8; 0x2b] = [0x54, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6B, 0x20, 0x62, 0x72,
                           0x6F, 0x77, 0x6E, 0x20, 0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70,
                           0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6C,
                           0x61, 0x7A, 0x79, 0x20, 0x64, 0x6F, 0x67];
  let mut index: usize = 0;
  for item in unsafe{quick_fox_output[0..176128].iter()} {
    assert_eq!(*item, fox[index]);
    index += 1;
    if index == 0x2b {
      index = 0;
    }
  }
}

static mut quick_fox_exported_output: [u8; QF_BUFFER_SIZE * 3] = [0u8; QF_BUFFER_SIZE * 3];
#[test]
fn test_quickfox_repeated_exported() {
  let input: [u8; 58] =
    [0x5B, 0xFF, 0xAF, 0x02, 0xC0, 0x22, 0x79, 0x5C, 0xFB, 0x5A, 0x8C, 0x42, 0x3B, 0xF4, 0x25,
     0x55, 0x19, 0x5A, 0x92, 0x99, 0xB1, 0x35, 0xC8, 0x19, 0x9E, 0x9E, 0x0A, 0x7B, 0x4B, 0x90,
     0xB9, 0x3C, 0x98, 0xC8, 0x09, 0x40, 0xF3, 0xE6, 0xD9, 0x4D, 0xE4, 0x6D, 0x65, 0x1B, 0x27,
     0x87, 0x13, 0x5F, 0xA6, 0xE9, 0x30, 0x96, 0x7B, 0x3C, 0x15, 0xD8, 0x53, 0x1C];
  let res = ::brotli_decode(&input[..], unsafe{&mut quick_fox_exported_output[..]});
  match res.result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  assert_eq!(res.decoded_size, 176128);
  const fox: [u8; 0x2b] = [0x54, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6B, 0x20, 0x62, 0x72,
                           0x6F, 0x77, 0x6E, 0x20, 0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70,
                           0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6C,
                           0x61, 0x7A, 0x79, 0x20, 0x64, 0x6F, 0x67];
  let mut index: usize = 0;
  for item in unsafe{quick_fox_exported_output[0..176128].iter()} {
    assert_eq!(*item, fox[index]);
    index += 1;
    if index == 0x2b {
      index = 0;
    }
  }
}

static mut quick_fox_prealloc_output: [u8; QF_BUFFER_SIZE * 3] = [0u8; QF_BUFFER_SIZE * 3];
#[test]
fn test_quickfox_repeated_exported_prealloc() {
  let input: [u8; 58] =
    [0x5B, 0xFF, 0xAF, 0x02, 0xC0, 0x22, 0x79, 0x5C, 0xFB, 0x5A, 0x8C, 0x42, 0x3B, 0xF4, 0x25,
     0x55, 0x19, 0x5A, 0x92, 0x99, 0xB1, 0x35, 0xC8, 0x19, 0x9E, 0x9E, 0x0A, 0x7B, 0x4B, 0x90,
     0xB9, 0x3C, 0x98, 0xC8, 0x09, 0x40, 0xF3, 0xE6, 0xD9, 0x4D, 0xE4, 0x6D, 0x65, 0x1B, 0x27,
     0x87, 0x13, 0x5F, 0xA6, 0xE9, 0x30, 0x96, 0x7B, 0x3C, 0x15, 0xD8, 0x53, 0x1C];
  let (qf, scratch) = unsafe{quick_fox_prealloc_output.split_at_mut(QF_BUFFER_SIZE)};
  let res = ::brotli_decode_prealloc(&input[..], qf, scratch, &mut[0u32;65536][..], &mut[HuffmanCode::default();65536][..]);
  match res.result {
    BrotliResult::ResultSuccess => {}
    _ => assert!(false),
  }
  assert_eq!(res.decoded_size, 176128);
  const fox: [u8; 0x2b] = [0x54, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6B, 0x20, 0x62, 0x72,
                           0x6F, 0x77, 0x6E, 0x20, 0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70,
                           0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6C,
                           0x61, 0x7A, 0x79, 0x20, 0x64, 0x6F, 0x67];
  let mut index: usize = 0;
  for item in qf[0..176128].iter() {
    assert_eq!(*item, fox[index]);
    index += 1;
    if index == 0x2b {
      index = 0;
    }
  }
}


#[cfg(feature="std")]
struct Buffer {
  data: Vec<u8>,
  read_offset: usize,
}
#[cfg(feature="std")]
impl Buffer {
  pub fn new(buf: &[u8]) -> Buffer {
    let mut ret = Buffer {
      data: Vec::<u8>::new(),
      read_offset: 0,
    };
    ret.data.extend(buf);
    return ret;
  }
}
#[cfg(feature="std")]
impl io::Read for Buffer {
  fn read(self: &mut Self, buf: &mut [u8]) -> io::Result<usize> {
    let bytes_to_read = ::core::cmp::min(buf.len(), self.data.len() - self.read_offset);
    if bytes_to_read > 0 {
      buf[0..bytes_to_read]
        .clone_from_slice(&self.data[self.read_offset..self.read_offset + bytes_to_read]);
    }
    self.read_offset += bytes_to_read;
    return Ok(bytes_to_read);
  }
}
#[cfg(feature="std")]
impl io::Write for Buffer {
  fn write(self: &mut Self, buf: &[u8]) -> io::Result<usize> {
    self.data.extend(buf);
    return Ok(buf.len());
  }
  fn flush(self: &mut Self) -> io::Result<()> {
    return Ok(());
  }
}


#[test]
#[cfg(feature="std")]
fn test_reader_quickfox_repeated() {
  let in_buf: [u8; 58] = [0x5B, 0xFF, 0xAF, 0x02, 0xC0, 0x22, 0x79, 0x5C, 0xFB, 0x5A, 0x8C, 0x42,
                          0x3B, 0xF4, 0x25, 0x55, 0x19, 0x5A, 0x92, 0x99, 0xB1, 0x35, 0xC8, 0x19,
                          0x9E, 0x9E, 0x0A, 0x7B, 0x4B, 0x90, 0xB9, 0x3C, 0x98, 0xC8, 0x09, 0x40,
                          0xF3, 0xE6, 0xD9, 0x4D, 0xE4, 0x6D, 0x65, 0x1B, 0x27, 0x87, 0x13, 0x5F,
                          0xA6, 0xE9, 0x30, 0x96, 0x7B, 0x3C, 0x15, 0xD8, 0x53, 0x1C];

  let mut output = Buffer::new(&[]);
  let mut input = super::Decompressor::new(Buffer::new(&in_buf), 4096);
  match super::copy_from_to(&mut input, &mut output) {
    Ok(_) => {}
    Err(e) => panic!("Error {:?}", e),
  }

  assert_eq!(output.data.len(), 176128);
  const fox: [u8; 0x2b] = [0x54, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6B, 0x20, 0x62, 0x72,
                           0x6F, 0x77, 0x6E, 0x20, 0x66, 0x6F, 0x78, 0x20, 0x6A, 0x75, 0x6D, 0x70,
                           0x73, 0x20, 0x6F, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6C,
                           0x61, 0x7A, 0x79, 0x20, 0x64, 0x6F, 0x67];
  let mut index: usize = 0;
  for item in output.data[0..176128].iter_mut() {
    assert_eq!(*item, fox[index]);
    index += 1;
    if index == 0x2b {
      index = 0;
    }
  }
}

#[test]
fn test_early_eof() {
  const BUFFER_SIZE: usize = 128;
  let mut input: [u8; 47] = [17, 17, 32, 32, 109, 109, 32, 32, 32, 181, 2, 0, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 151, 32, 42, 181, 32, 149, 59, 0, 0, 0, 0, 42, 42, 42, 42, 42, 5, 255, 255, 255, 255, 255];
  let mut output = [0u8; BUFFER_SIZE];
  let (result, input_offset, _output_offset) = oneshot(&mut input[..], &mut output[..]);
  match result {
    BrotliResult::ResultFailure => {}
    _ => assert!(false),
  }
  assert_eq!(input_offset, input.len());
}

#[test]
#[cfg(feature="std")]
fn test_run_out_of_writer_space() {
  // this is a valid compression of [0u8; 2048];
  let compression = [27, 255, 7, 0, 36, 0, 194, 177, 64, 114, 7];
  // output buffer doesn't have enough space
  let mut output_buffer = [0u8; 2047];

  super::BrotliDecompress(
    &mut io::Cursor::new(compression),
    &mut io::Cursor::new(&mut output_buffer[..]),
  )
  .unwrap_err();
}
