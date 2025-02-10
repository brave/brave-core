const TEXT: &[u8] = include_bytes!("../assets/example.txt");

#[test]
#[should_panic]
fn test_issue_182() {
    use std::io::BufRead;

    let compressed = zstd::encode_all(TEXT, 3).unwrap();
    let truncated = &compressed[..compressed.len() / 2];

    let rdr = zstd::Decoder::new(truncated).unwrap();
    let rdr = std::io::BufReader::new(rdr);
    for line in rdr.lines() {
        line.unwrap();
    }
}
