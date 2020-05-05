use std::env;
use std::fs::File;
use std::io::Write;
use std::path::Path;

extern crate regex;
use regex::Regex;

// use crate::classifier::{N_FEATURES, N_CLASSES};

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("predictor.rs");
    let mut f = File::create(&dest_path).unwrap();

    let model = include_str!("src/classifier/model.c");

    let main_remove = Regex::new(
        r###"(?ms)^int main\(int argc, const char \* argv\[\]\) \{
.*
\s+return 0;
}"###,
    )
    .unwrap();

    let transformed = main_remove.replace(model, "");

    let predictor_f_regex = Regex::new(
        r"(?m)^int predict_(?P<predictor>\d+)\(float features\[\]\) \{
\s+int classes\[\d+\];
(?P<body>(\s+.*\n)*\s+\})
\s+int class_idx = 0;
\s+int class_val = classes\[0\];
\s+int i;
\s+for \(i = \d+; i < \d+; i\+\+\) \{
\s+if \(classes\[i\] > class_val\) \{
\s+class_idx = i;
\s+class_val = classes\[i\];
\s+\}
\s+\}
\s+return class_idx;
\}$
",
    )
    .unwrap();

    let mut predictors: Vec<_> = Vec::new();
    for predictor in predictor_f_regex.captures_iter(&transformed) {
        let predictor_id_str = &predictor["predictor"];
        let predictor_id = predictor_id_str.parse::<u32>().unwrap();
        println!("Predictor {}", predictor_id);
        let body = &predictor["body"];
        println!(
            "Got predictor {} starting with {}, ending {}, len {}",
            predictor_id,
            &body[..100],
            &body[body.len() - 100..],
            body.len()
        );
        predictors.push((predictor_id, body.to_owned()));
    }

    let generated_predictors = predictors
        .iter()
        .map(|(p_id, body)| {
            format!(
                r###"
fn predict_{id}(features: &[f32; N_FEATURES])-> usize {{
    let mut classes: [i32; N_CLASSES] = Default::default();

    {body}

    let mut class_idx  = 0;
    let mut class_val = classes[0];
    for i in 0..N_CLASSES {{
        if classes[i] > class_val {{
            class_idx = i;
            class_val = classes[i];
        }}
    }}
    class_idx
}}
"###,
                id = p_id,
                body = body
            )
        })
        .collect::<Vec<_>>()
        .join("\n");

    let f_predict_body = predictors
        .iter()
        .map(|(p_id, _)| {
            format!(
                r###"
    let p{id} = predict_{id}(features);
    classes[p{id}] = classes[p{id}] + 1;
    "###,
                id = p_id
            )
        })
        .collect::<Vec<_>>()
        .join("\n");

    let generated = format!(
        r###"

pub const N_FEATURES: usize = {features};
pub const N_CLASSES: usize = {classes};

pub fn predict(features: &[f32; N_FEATURES])-> usize {{
    let mut classes: [i32; N_CLASSES] = Default::default();
    {f_predict_body}

    let mut class_idx: usize = 0;
    let mut class_val = classes[0];
    for i in 0..N_CLASSES {{
        if classes[i] > class_val {{
            class_idx = i;
            class_val = classes[i];
        }}
    }}
    class_idx
}}

{predictors}
    "###,
        features = 21,
        classes = 2,
        f_predict_body = &f_predict_body,
        predictors = &generated_predictors
    );

    f.write_all(&generated.as_bytes()).unwrap();
}
