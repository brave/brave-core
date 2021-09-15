use thiserror::Error;
use url::Url;

pub use lol_html::OutputSink;

use super::speedreader_heuristics::SpeedReaderHeuristics;
use super::speedreader_readability::SpeedReaderReadability;

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
    #[error("Processor already closed")]
    ProcessorClosed,
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

impl From<serde_json::error::Error> for SpeedReaderError {
    fn from(err: serde_json::error::Error) -> Self {
        SpeedReaderError::DeserializationError(err.to_string())
    }
}

impl From<std::str::Utf8Error> for SpeedReaderError {
    fn from(err: std::str::Utf8Error) -> Self {
        SpeedReaderError::DeserializationError(err.to_string())
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
#[repr(u8)]
pub enum RewriterType {
    Heuristics = 1,
    Readability = 2,
    Unknown = 3,
}

impl From<u8> for RewriterType {
    fn from(r_type: u8) -> Self {
        match r_type {
            1 => RewriterType::Heuristics,
            2 => RewriterType::Readability,
            _ => RewriterType::Unknown,
        }
    }
}

pub trait SpeedReaderProcessor {
    fn write(&mut self, input: &[u8]) -> Result<(), SpeedReaderError>;
    fn end(&mut self) -> Result<(), SpeedReaderError>;
    fn rewriter_type(&self) -> RewriterType;
}

#[derive(Default)]
pub struct SpeedReader;

impl SpeedReader {
    pub fn get_rewriter<'h, O: OutputSink + 'h>(
        &'h self,
        article_url: &str,
        output_sink: O,
        rewriter_type: RewriterType,
    ) -> Result<Box<dyn SpeedReaderProcessor + 'h>, SpeedReaderError> {
        if let Ok(url) = Url::parse(article_url) {
            match rewriter_type {
                RewriterType::Readability => {
                    Ok(Box::new(SpeedReaderReadability::try_new(url, output_sink)?))
                }
                _ => Ok(Box::new(SpeedReaderHeuristics::try_new(
                    url.as_str(),
                    output_sink,
                )?)),
            }
        } else {
            Err(SpeedReaderError::BadURL(article_url.to_owned()))
        }
    }
}
