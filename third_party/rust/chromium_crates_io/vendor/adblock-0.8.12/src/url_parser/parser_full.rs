use url::{Url};

fn parse_url(url: &str) -> Option<Url> {
    url.parse::<Url>()
    .ok() // convert to Option
}

pub fn get_url_host(url: &str) -> Option<String> {
    parse_url(url)
    .and_then(|p| p.host_str().map(String::from))
}
