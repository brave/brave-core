mod decoder;
mod encoder;

fn main() {
    let encoder = encoder::Encoder;
    let decoder = decoder::Decoder;
    let mut harness = toml_test_harness::EncoderHarness::new(encoder, decoder);
    harness
        .ignore([
            "valid/spec/float-0.toml", // Test issue; `Decoder` turns `6.626e-34` into `0.0`
        ])
        .unwrap();
    harness.test();
}
