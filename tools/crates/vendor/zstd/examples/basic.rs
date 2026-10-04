fn main() {
    let some_content = "Something";
    let compression_level = 3;

    // Compress some text
    let compressed =
        zstd::encode_all(some_content.as_bytes(), compression_level).unwrap();

    // Now uncompress it
    let decoded: Vec<u8> = zstd::decode_all(compressed.as_slice()).unwrap();

    // Convert it to text
    let decoded_text = std::str::from_utf8(&decoded).unwrap();

    assert_eq!(some_content, decoded_text);
}
