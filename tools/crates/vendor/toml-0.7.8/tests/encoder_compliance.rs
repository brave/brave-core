mod decoder;
mod encoder;

#[cfg(all(feature = "parse", feature = "display"))]
fn main() {
    let encoder = encoder::Encoder;
    let decoder = decoder::Decoder;
    let mut harness = toml_test_harness::EncoderHarness::new(encoder, decoder);
    harness.ignore(["valid/spec/float-0.toml"]).unwrap();
    harness.test();
}

#[cfg(not(all(feature = "parse", feature = "display")))]
fn main() {}
