extern crate speedreader;

use readability::extractor::extract_dom;

use speedreader::classifier::feature_extractor::FeatureExtractorStreamer;
use std::fs;
use url::Url;

fn main() {
    let url = Url::parse("http://example.com/hello/world/hello?again").unwrap();
    let frag1 = fs::read_to_string("data/classifier/bbc_new.html").expect("err to string");
    let frag2 = fs::read_to_string("data/classifier/bbc_new1.html").expect("err to string");
    let frag3 = fs::read_to_string("data/classifier/bbc_new2.html").expect("err to string");

    let mut streamer = FeatureExtractorStreamer::try_new(&url).unwrap();

    streamer.write(&mut frag1.as_bytes()).unwrap();
    streamer.write(&mut frag2.as_bytes()).unwrap();
    streamer.write(&mut frag3.as_bytes()).unwrap();

    let sink = streamer.end();

    println!("======\n Features returned sink:");
    let result = sink.features.clone();
    for (k, v) in result.iter() {
        println!("{}: {}", k, v)
    }

    let product = extract_dom(&mut sink.rcdom, &url, &sink.features).unwrap();
    println!(">> Read mode:\n {:?}", product);
    // parsing errors may happen due to malformed HTML, but it will not affect
    // parsing itself or the DOM tree building
    println!("Errors: {:?}", sink.rcdom.errors);
}
