extern crate readability;
extern crate reqwest;
extern crate url;

use readability::extractor::extract;
use regex::Regex;
use std::fs::{self, File};
use std::io::Write;
use std::process::Command;
use url::Url;

// This stylesheet is the one used by the crx packager.
static SPEEDREADER_CSS: &str = "https://raw.githubusercontent.com/brave-experiments/SpeedReader/master/data/content-stylesheet.css";
static SAMPLES_PATH: &str = "../../data/tests-samples";
static HTML_SLIDESHOW: &str = r#"
<html>
<head>
<style type="text/css">
  body, html {
    margin: 0; padding: 0; height: 100%; overflow: hidden;
  }

  /* Next & previous buttons */
  .prev, .next {
    cursor: pointer;
    position: absolute;
    top: 50%;
    width: auto;
    margin-top: -22px;
    padding: 16px;
    font-weight: bold;
    font-size: 18px;
    transition: 0.6s ease;
    border-radius: 0 3px 3px 0;
    user-select: none;
    z-index: 1000;
  }

  /* Position the "next button" to the right */
  .next {
    right: 0;
    border-radius: 3px 0 0 3px;
  }

  /* On hover, add a black background color with a little bit see-through */
  .prev:hover, .next:hover {
    background-color: rgba(0,0,0,0.8);
  }
</style>

<script>
var idx = 0;
const files = [
  {{files}}
];

const nextSlide = function(i) {
  const doc = document.getElementById("slide");
  idx = (idx + i) % files.length;
  doc.src = files[idx];
}

window.onload = function() {
  const doc = document.getElementById("slide");
  doc.src = files[0];
};

document.onkeydown = function(e) {
  switch (e.keyCode) {
  case 37:
    /* Left key pressed */
    nextSlide(-1);
    break;
  case 39:
    /* Right key pressed */
    nextSlide(1);
    break;
  }
};
</script>
</head>

<body>
  <!-- Next and previous buttons -->
  <a class="prev" onclick="nextSlide(-1)">&#10094;</a>
  <a class="prev next" onclick="nextSlide(1)">&#10095;</a>
  <iframe id="slide" width="100%" height="100%" frameborder="0" src="">
</body>
</html>
"#;

fn extract_wrap(name: &str, files: &mut Vec<String>, css: &[u8]) {
    let url = Url::parse("http://url.com").unwrap();
    let source_s = format!("{}/{}/source.html", SAMPLES_PATH, name);
    let mut source_f = File::open(source_s.to_string()).unwrap();

    let dest_s = format!("out/{}.html", name);
    let product = extract(&mut source_f, Some(url.as_str())).unwrap();
    let mut output_file = fs::File::create(dest_s.to_string()).unwrap();
    output_file.write_all(css).unwrap();
    output_file.write_all(product.content.as_bytes()).unwrap();

    files.push(format!(
        "\"{}\"",
        fs::canonicalize(source_s.to_string())
            .unwrap()
            .into_os_string()
            .into_string()
            .unwrap()
    ));
    files.push(format!(
        "\"{}\"",
        fs::canonicalize(dest_s.to_string())
            .unwrap()
            .into_os_string()
            .into_string()
            .unwrap()
    ));
}

fn main() {
    // Runs the extractor on article files and puts them side-by-side in the browser, so the user
    // can manually check for any regressions.
    let client = reqwest::blocking::Client::new();
    let sheet = format!(
        "<style id=\"brave_speedreader_style\">{}</style>",
        client.get(SPEEDREADER_CSS).send().unwrap().text().unwrap()
    );
    let css = sheet.as_bytes();
    let js_array_template: Regex = Regex::new(r"\{\{files\}\}").unwrap();
    fs::create_dir_all("out").unwrap();
    let mut files = vec![];

    extract_wrap("dutchnews_1", &mut files, &css);
    extract_wrap("nytimes_1", &mut files, &css);
    extract_wrap("cnet_2", &mut files, &css);
    extract_wrap("medium_2", &mut files, &css);
    extract_wrap("vanityfair_1", &mut files, &css);
    extract_wrap("bbc_2", &mut files, &css);
    extract_wrap("kotaku_1", &mut files, &css);
    extract_wrap("atlantic_1", &mut files, &css);
    extract_wrap("huffingtonpost_1", &mut files, &css);
    extract_wrap("rpp_1", &mut files, &css);
    extract_wrap("aclu", &mut files, &css);
    extract_wrap("ars_1", &mut files, &css);
    extract_wrap("bbc_1", &mut files, &css);
    extract_wrap("blogger", &mut files, &css);
    extract_wrap("bug_1255978", &mut files, &css);
    extract_wrap("buzzfeed_1", &mut files, &css);
    extract_wrap("guardian_1", &mut files, &css);
    extract_wrap("cnet", &mut files, &css);
    extract_wrap("graham_1", &mut files, &css);
    extract_wrap("cnn", &mut files, &css);
    extract_wrap("yahoo_1", &mut files, &css);
    extract_wrap("yahoo_2", &mut files, &css);
    extract_wrap("yahoo_3", &mut files, &css);
    extract_wrap("yahoo_4", &mut files, &css);

    let js_array = files.join(",\n  ");
    let cheap_template = js_array_template.replace(HTML_SLIDESHOW, js_array);
    let mut slideshow = fs::File::create("out/slideshow.html").unwrap();
    slideshow.write_all(cheap_template.as_bytes()).unwrap();

    let slideshow_path = fs::canonicalize("out/slideshow.html")
        .unwrap()
        .into_os_string()
        .into_string()
        .unwrap();
    Command::new("brave").arg(slideshow_path).spawn().unwrap();
}
