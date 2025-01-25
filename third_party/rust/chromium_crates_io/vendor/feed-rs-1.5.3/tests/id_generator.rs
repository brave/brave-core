use std::fs;
use std::path::PathBuf;

use feed_rs::parser;

// Verify we can generate IDs that are compatible with v0.2
#[test]
fn test_feed_ids_v0_2() {
    let mut test_file = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    test_file.push("fixture/rss2/rss_2.0_kdist.xml");
    let test_data = fs::read(test_file).unwrap();

    // Use the v0.2 ID generator
    let feed = parser::Builder::new().id_generator_v0_2().build().parse(test_data.as_slice()).unwrap();
    assert_eq!("7edcf1fbe86570753646f6eb75db4d55", feed.id);
}
