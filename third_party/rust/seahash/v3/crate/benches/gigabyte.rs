#![feature(test)]

extern crate test;
extern crate seahash;

#[bench]
fn gigabyte(b: &mut test::Bencher) {
    b.iter(|| {
        let mut x = 0;
        let mut buf = [15; 4096];

        for _ in 0..250000 {
            x ^= seahash::hash(&buf);
            buf[0] += buf[0].wrapping_add(1);
        }

        x
    })
}
