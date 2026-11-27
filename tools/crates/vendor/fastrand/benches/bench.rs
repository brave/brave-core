#![feature(test)]

extern crate test;

use rand::prelude::*;
use test::Bencher;

/// Exposes wyhash's RNG through the rand traits
struct WyRng(u64);

impl rand::TryRng for WyRng {
    type Error = std::convert::Infallible;

    fn try_next_u32(&mut self) -> Result<u32, Self::Error> {
        Ok(wyhash::wyrng(&mut self.0) as u32)
    }

    fn try_next_u64(&mut self) -> Result<u64, Self::Error> {
        Ok(wyhash::wyrng(&mut self.0))
    }

    fn try_fill_bytes(&mut self, dst: &mut [u8]) -> Result<(), Self::Error> {
        for chunk in dst.chunks_mut(8) {
            let bytes = wyhash::wyrng(&mut self.0).to_le_bytes();
            chunk.copy_from_slice(&bytes[..chunk.len()]);
        }
        Ok(())
    }
}

fn wyrng() -> WyRng {
    WyRng(rand::random())
}

#[bench]
fn shuffle_wyhash(b: &mut Bencher) {
    let mut rng = wyrng();
    let mut x = (0..100).collect::<Vec<usize>>();
    b.iter(|| {
        x.shuffle(&mut rng);
        x[0]
    })
}

#[bench]
fn shuffle_fastrand(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    let mut x = (0..100).collect::<Vec<usize>>();
    b.iter(|| {
        rng.shuffle(&mut x);
        x[0]
    })
}

#[bench]
fn u8_wyhash(b: &mut Bencher) {
    let mut rng = wyrng();
    b.iter(|| {
        let mut sum = 0u8;
        for _ in 0..10_000 {
            sum = sum.wrapping_add(rng.random::<u8>());
        }
        sum
    })
}

#[bench]
fn u8_fastrand(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    b.iter(|| {
        let mut sum = 0u8;
        for _ in 0..10_000 {
            sum = sum.wrapping_add(rng.u8(..));
        }
        sum
    })
}

#[bench]
fn u32_wyhash(b: &mut Bencher) {
    let mut rng = wyrng();
    b.iter(|| {
        let mut sum = 0u32;
        for _ in 0..10_000 {
            sum = sum.wrapping_add(rng.random::<u32>());
        }
        sum
    })
}

#[bench]
fn u32_fastrand(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    b.iter(|| {
        let mut sum = 0u32;
        for _ in 0..10_000 {
            sum = sum.wrapping_add(rng.u32(..));
        }
        sum
    })
}

#[bench]
fn f32_fastrand(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    b.iter(|| {
        // f32 sum unrolled 2x to hide f32-add latency.
        //
        // Optimal amount of unrolling is somewhat sensitive to CPU and algorithm.
        // Variously could be 2x, 3x, or 4x unrolling. On AArch64 and x86-64 on the
        // current algorithm, 2x seems to be optimal on this benchmark.
        let mut sum = 0.0;
        for _ in 0..5_000 {
            sum += rng.f32() + rng.f32();
        }
        sum
    })
}

#[bench]
fn fill(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    b.iter(|| {
        // Pick a size that isn't divisible by 8.
        let mut bytes = [0u8; 367];
        rng.fill(&mut bytes);
        bytes
    })
}

#[bench]
fn fill_naive(b: &mut Bencher) {
    let mut rng = fastrand::Rng::new();
    b.iter(|| {
        let mut bytes = [0u8; 367];
        for item in &mut bytes {
            *item = rng.u8(..);
        }
        bytes
    })
}
