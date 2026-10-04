mod decoder;

fn main() {
    let decoder = decoder::Decoder;
    let mut harness = toml_test_harness::DecoderHarness::new(decoder);
    harness
        .ignore([
            "valid/spec/float-0.toml", // Test issue; `Decoder` turns `6.626e-34` into `0.0`
            // Unreleased
            "valid/string/escape-esc.toml",
            "valid/string/hex-escape.toml",
            "valid/datetime/no-seconds.toml",
            "valid/inline-table/newline.toml",
        ])
        .unwrap();
    harness.test();
}
