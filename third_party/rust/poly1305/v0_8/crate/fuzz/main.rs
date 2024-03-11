#[macro_use]
extern crate afl;

fn main() {
    fuzz!(|data: &[u8]| {
        // Use first 32 bytes of data as key.
        if data.len() >= 32 {
            poly1305::fuzz_avx2((&data[0..32]).into(), &data[32..]);
        }
    });
}
