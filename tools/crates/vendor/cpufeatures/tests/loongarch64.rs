//! `loongarch64` tests

#![cfg(target_arch = "loongarch64")]

cpufeatures::new!(
    lacaps,
    "32s",
    "f",
    "d",
    "frecipe",
    "div32",
    "lsx",
    "lasx",
    "lam-bh",
    "lamcas",
    "ld-seq-sa",
    "scq",
    "lbt",
    "lvz",
    "ual"
);

#[test]
fn init() {
    let token: lacaps::InitToken = lacaps::init();
    assert_eq!(token.get(), lacaps::get());
}

#[test]
fn init_get() {
    let (token, val) = lacaps::init_get();
    assert_eq!(val, token.get());
}
