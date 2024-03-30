extern crate readability;
extern crate url;

use std::fs::File;
use url::Url;

#[test]
fn test_extract_title() {
    assert!(true);
    let mut file = File::open("./data/title.html").unwrap();
    let url = Url::parse("https://example.com").unwrap();
    let product = readability::extractor::extract(&mut file, &url).unwrap();
    assert_eq!(product.title, "This is title");
}
