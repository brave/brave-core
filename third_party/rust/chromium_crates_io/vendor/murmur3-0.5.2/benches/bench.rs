#![feature(test)]

extern crate murmur3_sys;
extern crate test;

use std::io::Cursor;
use test::Bencher;

extern crate murmur3;

use murmur3::*;

use murmur3_sys::MurmurHash3_x86_32;

#[bench]
fn bench_32(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        let mut tmp = Cursor::new(&string[0..string.len()]);
        murmur3_32(&mut tmp, 0)
    });
}

#[bench]
fn bench_c_32(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        unsafe {
            let output: [u8; 4] = [0; 4];
            MurmurHash3_x86_32(
                string.as_ptr() as _,
                string.len() as i32,
                0,
                output.as_ptr() as *mut _,
            );
            output[0]
        };
    });
}

#[bench]
fn bench_x86_128(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        let mut tmp = Cursor::new(&string[0..string.len()]);
        murmur3_x86_128(&mut tmp, 0)
    });
}

#[bench]
fn bench_c_x86_128(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        let output: [u8; 16] = [0; 16];
        unsafe {
            murmur3_sys::MurmurHash3_x86_128(
                string.as_ptr() as _,
                string.len() as i32,
                0,
                output.as_ptr() as *mut _,
            );
        }
        output[0]
    });
}

#[bench]
fn bench_x64_128(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        let mut tmp = Cursor::new(&string[0..string.len()]);
        murmur3_x64_128(&mut tmp, 0)
    });
}

#[bench]
fn bench_c_x64_128(b: &mut Bencher) {
    let string: &[u8] =
        test::black_box(b"Lorem ipsum dolor sit amet, consectetur adipisicing elit");
    b.bytes = string.len() as u64;
    b.iter(|| {
        let output: [u8; 16] = [0; 16];
        unsafe {
            murmur3_sys::MurmurHash3_x64_128(
                string.as_ptr() as _,
                string.len() as i32,
                0,
                output.as_ptr() as *mut _,
            );
        }
        output[0]
    });
}
