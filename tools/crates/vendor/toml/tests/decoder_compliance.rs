mod decoder;

#[cfg(all(feature = "parse", feature = "display"))]
fn main() {
    let decoder = decoder::Decoder;
    let mut harness = toml_test_harness::DecoderHarness::new(decoder);
    harness.version("1.0.0");
    harness.ignore([]).unwrap();
    harness.test();
}

#[cfg(not(all(feature = "parse", feature = "display")))]
fn main() {}
