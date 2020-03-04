extern crate speedreader;
extern crate url;

use readability::extractor::extract_dom;
use speedreader::classifier::feature_extractor::FeatureExtractorStreamer;
use speedreader::classifier::Classifier;
use std::fs;
use url::Url;

fn main() {
    let url = Url::parse("http://example.com/hello/world/hello?again").unwrap();
    let data = fs::read_to_string("./examples/html/bbc_1.html").expect("err to string");

    // feature extraction
    let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
    feature_extractor.write(&mut data.as_bytes()).unwrap();
    let result = feature_extractor.end();

    println!(">> Feature List");
    for (k, v) in result.features.to_owned().iter() {
        println!("{}: {}", k, v);
    }

    // document classification
    let classifier_result = Classifier::from_feature_map(&result.features).classify();
    println!(">> Readble?\n {}", classifier_result);

    // document mapper
    let product = extract_dom(&mut result.rcdom, &url, &result.features).unwrap();
    println!(">> Read mode:\n {}", product.content);
}
