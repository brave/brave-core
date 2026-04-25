#![feature(test)]
extern crate test;

use digest::bench_update;
use sha1::Sha1;
use sha1_checked::Sha1 as Sha1Checked;
use test::Bencher;

bench_update!(
    Sha1::default();
    sha1_10 10;
    sha1_100 100;
    sha1_1000 1000;
    sha1_10000 10000;
);

bench_update!(
    Sha1Checked::default();
    sha1_checked_10 10;
    sha1_checked_100 100;
    sha1_checked_1000 1000;
    sha1_checked_10000 10000;
);

bench_update!(
    Sha1Checked::builder().detect_collision(false).build();
    sha1_checked_no_check_10 10;
    sha1_checked_no_check_100 100;
    sha1_checked_no_check_1000 1000;
    sha1_checked_no_check_10000 10000;
);

bench_update!(
    Sha1Checked::builder().use_ubc(false).build();
    sha1_checked_no_ubc_10 10;
    sha1_checked_no_ubc_100 100;
    sha1_checked_no_ubc_1000 1000;
    sha1_checked_no_ubc_10000 10000;
);
