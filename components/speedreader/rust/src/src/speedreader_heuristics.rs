use lol_html::OutputSink;

use std::borrow::Borrow;
use std::cell::RefCell;
use url::Url;

use super::classifier::feature_extractor::{FeatureExtractorStreamer, FeaturisingTreeSink};
use super::classifier::Classifier;
use super::speedreader::*;

use readability::extractor;

pub struct SpeedReaderHeuristics<O>
where
    O: OutputSink,
{
    url: Option<Url>,
    readable: RefCell<Option<bool>>,
    streamer: FeatureExtractorStreamer,
    output_sink: O,
}

impl<O: OutputSink> SpeedReaderProcessor for SpeedReaderHeuristics<O> {
    fn write(&mut self, input: &[u8]) -> Result<(), SpeedReaderError> {
        if self.document_readable() != Some(false)
            && self.streamer.write(&mut input.borrow()).is_err()
        {
            *self.readable.borrow_mut() = Some(false)
        }
        // else NOOP - already decided the doc is not readable
        Ok(())
    }

    fn end(&mut self) -> Result<(), SpeedReaderError> {
        if let Some(url) = self.url.as_ref() {
            // Already decided the document is not readable
            if self.document_readable() == Some(false) {
                return Err(SpeedReaderError::RewritingError(
                    "Not readable with heuristics".to_owned(),
                ));
            }
            let (readable, maybe_doc) = process(self.streamer.end(), &url);

            *self.readable.borrow_mut() = Some(readable);
            if readable {
                if let Some(doc) = maybe_doc {
                    self.output_sink.handle_chunk(doc.as_bytes());
                    Ok(())
                } else {
                    Err(SpeedReaderError::RewritingError(
                        "Failed to extract content with heuristics".to_owned(),
                    ))
                }
            } else {
                Err(SpeedReaderError::RewritingError(
                    "Not readable with heuristics".to_owned(),
                ))
            }
        } else {
            // No valid URL - no document
            Err(SpeedReaderError::InvalidUrl("".to_owned()))
        }
    }

    fn rewriter_type(&self) -> RewriterType {
        RewriterType::Heuristics
    }
}

impl<O: OutputSink> SpeedReaderHeuristics<O> {
    pub fn try_new(url: &str, output_sink: O) -> Result<Self, SpeedReaderError> {
        let url_parsed = Url::parse(url);

        url_parsed.map(|url_parsed| {
            if url_maybe_readable(&url_parsed) {
                let streamer = FeatureExtractorStreamer::try_new(&url_parsed)?;
                Ok(SpeedReaderHeuristics {
                    url: Some(url_parsed),
                    readable: RefCell::new(None),
                    streamer,
                    output_sink,
                })
            } else {
                Err(SpeedReaderError::InvalidUrl(url.to_owned()))
            }
        })?
    }

    pub fn document_readable(&self) -> Option<bool> {
        *self.readable.borrow()
    }
}

fn process(sink: &mut FeaturisingTreeSink, url: &Url) -> (bool, Option<String>) {
    let class = Classifier::from_feature_map(&sink.features).classify();
    if class == 0 {
        (false, None)
    } else if let Ok(extracted) = extractor::extract_dom(&mut sink.rcdom, url, &sink.features) {
        (true, Some(extracted.content))
    } else {
        (false, None)
    }
}

fn url_maybe_readable(url: &Url) -> bool {
    let scheme = url.scheme();
    scheme == "http" || scheme == "https"
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_speedreader_streamer() {
        let mut buf = vec![];
        let mut sreader =
            SpeedReaderHeuristics::try_new("https://test.xyz", |c: &[u8]| buf.extend_from_slice(c))
                .unwrap();

        let buff1 = "<html><p>hello".as_bytes();
        let buff2 = "world </p>\n\n\n\n<br><br><a href='/link'>".as_bytes();
        let buff3 = "this is a link</a></html>".as_bytes();

        sreader.write(&buff1).ok();
        sreader.write(&buff2).ok();
        sreader.write(&buff3).ok();
        sreader.end().ok();
        let result_sink = sreader.streamer.end();

        assert_eq!(result_sink.features["url_depth"], 1);
        assert_eq!(result_sink.features["p"], 1);
        assert_eq!(result_sink.features["a"], 1);
    }
}
