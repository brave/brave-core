extern crate schnorrkel;
extern crate cxx;

#[allow(unused)]
#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = brave_wallet::polkadot)]
mod ffi {
    struct CxxPolkadotTest {
        value: u32,
    }

    extern "Rust" {
        fn rawr(_: CxxPolkadotTest) -> i32;
    }
}

fn rawr(_: ffi::CxxPolkadotTest) -> i32 { 1234 }
