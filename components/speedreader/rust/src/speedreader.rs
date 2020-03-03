use lol_html::Selector;
use serde::{Deserialize, Serialize};
use std::any::Any;
use thiserror::Error;
use url::Url;

pub use lol_html::OutputSink;

use super::rewriter_config_builder::*;
use super::speedreader_heuristics::SpeedReaderHeuristics;
use super::speedreader_streaming::SpeedReaderStreaming;
use super::whitelist::Whitelist;
use lol_html::errors::SelectorError;

#[derive(Error, Debug, PartialEq)]
pub enum SpeedReaderError {
    #[error("Invalid article URL.")]
    InvalidUrl(String),
    #[error("Document parsing error: `{0}`")]
    DocumentParseError(String),
    #[error("Document rewriting error: `{0}`")]
    RewritingError(String),
    #[error("Configuration error: `{0}`")]
    ConfigurationError(String),
    #[error("Serialization error: `{0}`")]
    SerializationError(String),
    #[error("Deserialization error: `{0}`")]
    DeserializationError(String),
    #[error("Bad URL: `{0}`")]
    BadURL(String),
    #[error("Selector Error: `{0}` - {1}")]
    SelectorError(String, SelectorError),
}

impl From<lol_html::errors::RewritingError> for SpeedReaderError {
    fn from(err: lol_html::errors::RewritingError) -> Self {
        SpeedReaderError::RewritingError(err.to_string())
    }
}

impl From<lol_html::errors::EncodingError> for SpeedReaderError {
    fn from(err: lol_html::errors::EncodingError) -> Self {
        SpeedReaderError::RewritingError(err.to_string())
    }
}

impl From<url::ParseError> for SpeedReaderError {
    fn from(err: url::ParseError) -> Self {
        SpeedReaderError::InvalidUrl(err.to_string())
    }
}

impl From<std::io::Error> for SpeedReaderError {
    fn from(err: std::io::Error) -> Self {
        SpeedReaderError::DocumentParseError(err.to_string())
    }
}

impl From<rmps::decode::Error> for SpeedReaderError {
    fn from(err: rmps::decode::Error) -> Self {
        SpeedReaderError::DeserializationError(err.to_string())
    }
}

impl From<rmps::encode::Error> for SpeedReaderError {
    fn from(err: rmps::encode::Error) -> Self {
        SpeedReaderError::SerializationError(err.to_string())
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
#[repr(u8)]
pub enum RewriterType {
    Streaming = 0,
    Heuristics = 1,
    Unknown = 2,
}

impl From<u8> for RewriterType {
    fn from(r_type: u8) -> Self {
        match r_type {
            0 => RewriterType::Streaming,
            1 => RewriterType::Heuristics,
            _ => RewriterType::Unknown,
        }
    }
}

pub trait SpeedReaderProcessor {
    fn write(&mut self, input: &[u8]) -> Result<(), SpeedReaderError>;
    fn end(&mut self) -> Result<(), SpeedReaderError>;
    fn rewriter_type(&self) -> RewriterType;
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct SpeedReaderConfig {
    pub domain: String,
    pub url_rules: Vec<String>,
    pub declarative_rewrite: Option<RewriteRules>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct AttributeRewrite {
    pub selector: String,
    pub attribute: String,
    pub to_attribute: String,
    pub element_name: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct RewriteRules {
    pub main_content: Vec<String>,
    pub main_content_cleanup: Vec<String>,
    pub delazify: bool,
    pub fix_embeds: bool,
    pub content_script: Option<String>,
    pub preprocess: Vec<AttributeRewrite>,
}

impl RewriteRules {
    pub fn get_content_handlers(&self, url: &Url) -> Vec<(Selector, ContentFunction)> {
        rewrite_rules_to_content_handlers(self, &url.origin().ascii_serialization())
    }
}

impl RewriteRules {
    pub fn get_main_content_selectors(&self) -> Vec<&str> {
        self.main_content.iter().map(AsRef::as_ref).collect()
    }
    pub fn get_content_cleanup_selectors(&self) -> Vec<&str> {
        self.main_content_cleanup
            .iter()
            .map(AsRef::as_ref)
            .collect()
    }
}

pub struct SpeedReader {
    whitelist: Whitelist,
    url_engine: adblock::engine::Engine,
}

impl Default for SpeedReader {
    fn default() -> Self {
        let mut whitelist = Whitelist::default();
        whitelist.load_predefined();
        let url_engine = adblock::engine::Engine::from_rules(&whitelist.get_url_rules());
        SpeedReader {
            whitelist,
            url_engine,
        }
    }
}

impl SpeedReader {
    pub fn with_whitelist(whitelist: Whitelist) -> Self {
        let url_engine = adblock::engine::Engine::from_rules(&whitelist.get_url_rules());
        SpeedReader {
            whitelist,
            url_engine,
        }
    }

    pub fn url_readable(&self, url: &str) -> Option<bool> {
        let matched = self.url_engine.check_network_urls(url, url, "");
        if matched.exception.is_some() {
            Some(false)
        } else if matched.matched {
            Some(true)
        } else {
            None
        }
    }

    pub fn get_rewriter_type(&self, article_url: &str) -> RewriterType {
        if let Ok(url) = Url::parse(article_url) {
            let config = self
                .whitelist
                .get_configuration(&url.domain().unwrap_or_default());

            match config {
                Some(SpeedReaderConfig {
                    declarative_rewrite: Some(_),
                    ..
                }) => RewriterType::Streaming,
                Some(_) => RewriterType::Heuristics,
                None => RewriterType::Unknown,
            }
        } else {
            RewriterType::Unknown
        }
    }

    pub fn get_opaque_config(&self, article_url: &str) -> Box<dyn Any> {
        if let Ok(url) = Url::parse(article_url) {
            let config = self
                .whitelist
                .get_configuration(&url.domain().unwrap_or_default());

            match config {
                Some(SpeedReaderConfig {
                    declarative_rewrite: Some(rewrite),
                    ..
                }) => Box::new(rewrite.get_content_handlers(&url)),
                _ => Box::new(Vec::<(Selector, ContentFunction)>::new()),
            }
        } else {
            Box::new(Vec::<(Selector, ContentFunction)>::new())
        }
    }

    pub fn get_rewriter<'h, O: OutputSink + 'h>(
        &'h self,
        article_url: &str,
        extra: &'h Box<dyn Any>,
        output_sink: O,
        rewriter_type: Option<RewriterType>,
    ) -> Result<Box<dyn SpeedReaderProcessor + 'h>, SpeedReaderError> {
        if let Ok(url) = Url::parse(article_url) {
            let rewriter_decided = match rewriter_type {
                Some(r_type) => r_type,
                None => self.get_rewriter_type(article_url),
            };

            if let Some(content_handlers) = extra.downcast_ref::<Vec<(Selector, ContentFunction)>>()
            {
                match rewriter_decided {
                    RewriterType::Streaming => Ok(Box::new(SpeedReaderStreaming::try_new(
                        url,
                        output_sink,
                        content_handlers,
                    )?)),
                    _ => Ok(Box::new(SpeedReaderHeuristics::try_new(
                        url.as_str(),
                        output_sink,
                    )?)),
                }
            } else {
                Err(SpeedReaderError::ConfigurationError(
                "The configuration `extra` parameter could not be unmarshalled to expected type"
                    .to_owned()))
            }
        } else {
            Err(SpeedReaderError::BadURL(article_url.to_owned()))
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    pub fn get_whitelist() -> Whitelist {
        let mut whitelist = Whitelist::default();
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.com".to_owned(),
            url_rules: vec![
                r#"||example.com/article"#.to_owned(),
                r#"@@||example.com/article/video"#.to_owned(),
            ],
            declarative_rewrite: None,
        });
        whitelist.add_configuration(SpeedReaderConfig {
            domain: "example.net".to_owned(),
            url_rules: vec![r#"||example.net/article"#.to_owned()],
            declarative_rewrite: Some(RewriteRules {
                main_content: vec!["article".to_owned()],
                main_content_cleanup: vec![],
                delazify: true,
                fix_embeds: true,
                content_script: None,
                preprocess: vec![],
            }),
        });
        whitelist
    }

    #[test]
    pub fn url_readable_matches() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let readable = sr.url_readable("http://example.com/article/today");
        assert_eq!(readable, Some(true));
    }

    #[test]
    pub fn url_readable_subdomain_matches() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let readable = sr.url_readable("http://subdomain.example.com/article/today");
        assert_eq!(readable, Some(true));
    }

    #[test]
    pub fn url_exception_matches() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let readable = sr.url_readable("http://example.com/article/video");
        assert_eq!(readable, Some(false));
    }

    #[test]
    pub fn url_no_match() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let readable = sr.url_readable("http://smart-e.org/blog");
        assert_eq!(readable, None);
    }

    #[test]
    pub fn configuration_matching_some() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let config = sr.get_rewriter_type("http://example.com/article/today");
        assert_eq!(config, RewriterType::Heuristics);
    }

    #[test]
    pub fn configuration_nomatch_none() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let config = sr.get_rewriter_type("http://bbc.com/article/today");
        assert_eq!(config, RewriterType::Unknown);
    }

    #[test]
    pub fn configuration_opaque_correctly_typed() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let article = "http://example.com/article/today";
        let config = sr.get_rewriter_type(article);
        assert_eq!(config, RewriterType::Heuristics);
        let opaque = sr.get_opaque_config(article);
        assert!(opaque
            .downcast_ref::<Vec<(Selector, ContentFunction)>>()
            .is_some());
    }

    #[test]
    pub fn rewriter_configured_heuristics() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let article = "http://example.com/article/today";
        let config = sr.get_rewriter_type(article);
        assert_eq!(config, RewriterType::Heuristics);
        let opaque = sr.get_opaque_config(article);
        let maybe_rewriter = sr.get_rewriter(article, &opaque, |_: &[u8]| {}, Some(config));
        assert!(maybe_rewriter.is_ok());
        let rewriter = maybe_rewriter.unwrap();
        assert_eq!(rewriter.rewriter_type(), RewriterType::Heuristics);
    }

    #[test]
    pub fn rewriter_configured_streaming() {
        let sr = SpeedReader::with_whitelist(get_whitelist());
        let article = "http://example.net/article/today";
        let config = sr.get_rewriter_type(article);
        assert_eq!(config, RewriterType::Streaming);
        let opaque = sr.get_opaque_config(article);
        let maybe_rewriter = sr.get_rewriter(article, &opaque, |_: &[u8]| {}, Some(config));
        assert!(maybe_rewriter.is_ok());
        let rewriter = maybe_rewriter.unwrap();
        assert_eq!(rewriter.rewriter_type(), RewriterType::Streaming);
    }
}
