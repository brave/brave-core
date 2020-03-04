extern crate reqwest;
extern crate speedreader;
extern crate url;

use readability::extractor::extract_dom;
use serde_json::json;
use speedreader::classifier::feature_extractor::FeatureExtractorStreamer;
use speedreader::classifier::Classifier;
use std::collections::hash_map::DefaultHasher;
use std::env;
use std::fs;
use std::hash::{Hash, Hasher};
use std::io::prelude::*;
use url::Url;

fn calculate_hash<T: Hash>(t: &T) -> u64 {
    let mut s = DefaultHasher::new();
    t.hash(&mut s);
    s.finish()
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let article_url = &args[1];

    let url = Url::parse(article_url).unwrap();

    let client = reqwest::blocking::Client::new();
    let data = client.get(article_url)
        .header(reqwest::header::COOKIE, "forbesbeta=A; forbes_t=%7B%22cd%22%3A39%7D; notice_behavior=expressed,eu; client_id=b3f483f643eea514579b7927fd8be579acc; notice_gdpr_prefs=0,1,2:1a8b5228dd7ff0717196863a5d28ce6c; notice_preferences=2:1a8b5228dd7ff0717196863a5d28ce6c; fadve2etid=P5525432051621425152; fadve2etidvcnt=5")
        .send()
        .unwrap()
        .text()
        .unwrap();

    let dir = format!(
        "data/pages/{}/{}",
        url.host().unwrap(),
        calculate_hash(article_url)
    );
    println!("Creating directory: {}", dir);
    fs::create_dir_all(&dir).unwrap_or_default();

    let filename_html = format!("{}/init.html", &dir);
    let mut file = fs::File::create(filename_html).unwrap();
    file.write_all(data.as_bytes()).unwrap();

    // feature extraction
    let mut feature_extractor = FeatureExtractorStreamer::try_new(&url).unwrap();
    feature_extractor.write(&mut data.as_bytes()).unwrap();
    let result = feature_extractor.end();

    // document classification
    let classifier_result = Classifier::from_feature_map(&result.features).classify();
    println!(">> Readble?\n {}", classifier_result);

    if classifier_result > 0 {
        // document mapper
        let product = extract_dom(&mut result.rcdom, &url, &result.features).unwrap();
        let filename_html = format!("{}/mapped.html", &dir);
        let mut file = fs::File::create(filename_html).unwrap();
        file.write_all(product.content.as_bytes()).unwrap();
    }

    // Metadata
    let metadata = json!({
        "host": url.host().unwrap().to_string(),
        "url": url.into_string(),
        "hash": calculate_hash(article_url),
        "readable": classifier_result,
        "correct": "tbd"
    });
    let filename_html = format!("{}/metadata.json", &dir);
    let mut file = fs::File::create(filename_html).unwrap();
    file.write_all(metadata.to_string().as_bytes()).unwrap();
}
