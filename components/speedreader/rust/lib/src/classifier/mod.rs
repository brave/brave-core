pub mod feature_extractor;
mod model;
use std::collections::HashMap;

use model::predict;
use model::N_FEATURES;

/// Hold a set of feature measurements and use them to predict readability.
pub struct Classifier {
    features_list: [f32; N_FEATURES],
}

impl Classifier {
    pub fn from_feature_map(features: &HashMap<String, u32>) -> Classifier {
        let features_list = convert_map(features);
        Classifier { features_list }
    }

    pub fn classify(&self) -> usize {
        predict(&self.features_list)
    }
}

/// Convert a feature map to a vector for model input.
fn convert_map(map: &HashMap<String, u32>) -> [f32; N_FEATURES] {
    let mut slice: [f32; N_FEATURES] = [0.0; N_FEATURES];

    // These must be the same order as the data the model was trained on!
    const KEYS: [&str; N_FEATURES] = [
        "img",
        "a",
        "script",
        "text_blocks",
        "words",
        "blockquote",
        "dl",
        "div",
        "ol",
        "p",
        "pre",
        "table",
        "ul",
        "select",
        "article",
        "section",
        "url_depth",
        "amphtml",
        "fb_pages",
        "og_article",
        "schema_org",
        /*"file_size",*/
    ];

    // Fill each slot in the features vector with the corresponding
    // value from `map`.
    for (feature_slot, key) in slice.iter_mut().zip(KEYS) {
        *feature_slot = map.get(key).cloned().unwrap_or(0) as f32;
    }

    slice
}
