#![no_main]
use core::convert::TryFrom;
use libfuzzer_sys::fuzz_target;
use sharks::Share;

fuzz_target!(|data: &[u8]| {
    let _share = Share::try_from(data);
});
