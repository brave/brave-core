#![feature(test)]
extern crate test;

use digest::bench_update;
use ripemd::{Ripemd128, Ripemd160, Ripemd256, Ripemd320};
use test::Bencher;

bench_update!(
    Ripemd128::default();
    ripemd128_10 10;
    ripemd128_100 100;
    ripemd128_1000 1000;
    ripemd128_10000 10000;
);

bench_update!(
    Ripemd160::default();
    ripemd160_10 10;
    ripemd160_100 100;
    ripemd160_1000 1000;
    ripemd160_10000 10000;
);

bench_update!(
    Ripemd256::default();
    ripemd256_10 10;
    ripemd256_100 100;
    ripemd256_1000 1000;
    ripemd256_10000 10000;
);

bench_update!(
    Ripemd320::default();
    ripemd320_10 10;
    ripemd320_100 100;
    ripemd320_1000 1000;
    ripemd320_10000 10000;
);
