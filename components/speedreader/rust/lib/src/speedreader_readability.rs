use html5ever::driver::{ParseOpts, Parser};
use html5ever::tendril::{StrTendril, TendrilSink};
use kuchiki::Sink;
use lol_html::OutputSink;
use std::collections::HashMap;
use url::Url;

use super::speedreader::*;

use readability::{extractor, statistics};

const MOZ_SCORE_THRESHOLD: f64 = 30.0;

pub struct SpeedReaderReadability<O>
where
    O: OutputSink,
{
    min_out_length: Option<i32>,
    parser: Option<Parser<Sink>>,
    url: Url,
    output_sink: O,
}

impl<O: OutputSink> SpeedReaderProcessor for SpeedReaderReadability<O> {
    fn set_min_out_length(&mut self, min_out_length: i32) {
        self.min_out_length = Some(min_out_length);
    }

    fn write(&mut self, input: &[u8]) -> Result<(), SpeedReaderError> {
        if let Some(ref mut parser) = self.parser {
            match StrTendril::try_from_byte_slice(input) {
                Ok(tendril) => {
                    parser.process(tendril);
                    Ok(())
                }
                Err(_) => {
                    Err(SpeedReaderError::DocumentParseError("Could not write tendril".to_owned()))
                }
            }
        } else {
            Err(SpeedReaderError::ProcessorClosed)
        }
    }

    fn end(&mut self) -> Result<(), SpeedReaderError> {
        if let Some(parser) = self.parser.take() {
            let mut dom: Sink = parser.finish();
            if let Some(features) = statistics::collect_statistics(&dom) {
                if features.moz_score > 20.0 {
                    let extracted = extractor::extract_dom(&mut dom, &self.url, self.min_out_length, &HashMap::new())?;
                    self.output_sink.handle_chunk(extracted.content.as_bytes());
                    Ok(())
                } else {
                    Err(SpeedReaderError::RewritingError(
                        format!("Not readable. Moz score: {}", features.moz_score).to_owned(),
                    ))
                }
            } else {
                Err(SpeedReaderError::RewritingError("Could not score document".to_owned()))
            }
        } else {
            Err(SpeedReaderError::ProcessorClosed)
        }
    }

    #[inline]
    fn rewriter_type(&self) -> RewriterType {
        RewriterType::Readability
    }
}

impl<O: OutputSink> SpeedReaderReadability<O> {
    pub fn try_new(url: Url, output_sink: O) -> Result<Self, SpeedReaderError> {
        let scheme = url.scheme();

        if scheme == "http" || scheme == "https" {
            let parser = html5ever::parse_document(Sink::default(), ParseOpts::default());
            Ok(SpeedReaderReadability {
                min_out_length: None,
                parser: Some(parser),
                url,
                output_sink,
            })
        } else {
            Err(SpeedReaderError::InvalidUrl(url.to_string()))
        }
    }
}
