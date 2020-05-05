pub mod feature_extractor;
mod model;
use std::collections::HashMap;

use model::predict;
use model::N_FEATURES;

pub struct Classifier {
    features_list: [f32; N_FEATURES],
}

impl Classifier {
    pub fn from_feature_map(features: &HashMap<String, u32>) -> Classifier {
        //let features_list: [f32; N_FEATURES] = [0.0; N_FEATURES];

        let features_list = convert_map(features);
        Classifier { features_list }
    }

    pub fn classify(&self) -> usize {
        predict(&self.features_list)
    }
}

// helpers
fn convert_map(map: &HashMap<String, u32>) -> [f32; N_FEATURES] {
    let mut slice: [f32; N_FEATURES] = [0.0; N_FEATURES];

    slice[0] = map.get("img").cloned().unwrap_or(0) as f32;
    slice[1] = map.get("a").cloned().unwrap_or(0) as f32;
    slice[2] = map.get("script").cloned().unwrap_or(0) as f32;
    slice[3] = map.get("text_blocks").cloned().unwrap_or(0) as f32;
    slice[4] = map.get("words").cloned().unwrap_or(0) as f32;
    slice[5] = map.get("blockquote").cloned().unwrap_or(0) as f32;
    slice[6] = map.get("dl").cloned().unwrap_or(0) as f32;
    slice[7] = map.get("div").cloned().unwrap_or(0) as f32;
    slice[8] = map.get("ol").cloned().unwrap_or(0) as f32;
    slice[9] = map.get("p").cloned().unwrap_or(0) as f32;
    slice[10] = map.get("pre").cloned().unwrap_or(0) as f32;
    slice[11] = map.get("table").cloned().unwrap_or(0) as f32;
    slice[12] = map.get("ul").cloned().unwrap_or(0) as f32;
    slice[13] = map.get("select").cloned().unwrap_or(0) as f32;
    slice[14] = map.get("article").cloned().unwrap_or(0) as f32;
    slice[15] = map.get("section").cloned().unwrap_or(0) as f32;
    slice[16] = map.get("url_depth").cloned().unwrap_or(0) as f32;
    slice[17] = map.get("amphtml").cloned().unwrap_or(0) as f32;
    slice[18] = map.get("fb_pages").cloned().unwrap_or(0) as f32;
    slice[19] = map.get("og_article").cloned().unwrap_or(0) as f32;
    slice[20] = map.get("schema_org").cloned().unwrap_or(0) as f32;
    //slice[21] = map.get("file_size").cloned().unwrap_or(0) as f32;

    slice
}
