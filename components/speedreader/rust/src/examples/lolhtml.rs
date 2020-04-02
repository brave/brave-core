extern crate reqwest;
extern crate speedreader;
extern crate url;

use std::collections::hash_map::DefaultHasher;
use std::env;
use std::error::Error;
use std::fs;
use std::hash::{Hash, Hasher};
use std::io::prelude::*;
use url::Url;

use speedreader::*;

fn calculate_hash<T: Hash>(t: &T) -> u64 {
    let mut s = DefaultHasher::new();
    t.hash(&mut s);
    s.finish()
}

const USER_AGENT: &'static str = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.106 Safari/537.36";

async fn stream_content(article_url: &str) -> Result<(), Box<dyn Error>> {
    let url = Url::parse(article_url).unwrap();

    let dir = format!(
        "data/lolhtml/{}/{}",
        url.host().unwrap(),
        calculate_hash(&article_url)
    );
    println!("Creating directory: {}", dir);
    fs::create_dir_all(&dir).unwrap_or_default();

    let client = reqwest::Client::new();
    let mut data = client
        .get(url.as_str())
        .header("cookie", r#""#)
        .header("referer", "https://www.usnews.com/")
        .header("sec-fetch-dest", "document")
        .header("sec-fetch-mode", "navigate")
        .header("sec-fetch-site", "same-origin")
        .header("sec-fetch-user", "?1")
        .header("user-agent", USER_AGENT)
        .send()
        .await?;

    let mut mapped_file = fs::File::create(format!("{}/mapped.html", &dir))?;
    let mut mapped_test_file =
        fs::File::create(format!("{}/mapped.html", "data/lolhtml/test"))?;

    let sr = SpeedReader::default();
    let config = sr.get_rewriter_type(article_url);
    let opaque = sr.get_opaque_config(article_url);
    let mut rewriter = sr.get_rewriter(
        article_url,
        &opaque,
        |c: &[u8]| {
            mapped_file.write_all(c).ok();
            mapped_test_file.write_all(c).ok();
        },
        Some(config),
    )?;

    let mut init_file = fs::File::create(format!("{}/init.html", &dir))?;
    let mut init_test_file = fs::File::create(format!("{}/init.html", "data/lolhtml/test"))?;
    while let Some(chunk) = data.chunk().await? {
        rewriter.write(chunk.as_ref()).ok();
        init_file.write_all(chunk.as_ref()).ok();
        init_test_file.write_all(chunk.as_ref()).ok();
    }

    rewriter.end()?;

    Ok(())
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = env::args().collect();
    let article_url = &args[1];
    stream_content(article_url).await?;

    Ok(())
}
