#![no_main]
use libfuzzer_sys::fuzz_target;

use sharks::Share;

fuzz_target!(|share: Share| {
    let _data: Vec<u8> = (&share).into();
});
