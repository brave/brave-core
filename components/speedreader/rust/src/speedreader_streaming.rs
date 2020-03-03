use lol_html::doc_comments;
use lol_html::OutputSink;
use lol_html::Selector;
use lol_html::{HtmlRewriter, Settings};
use url::Url;

use super::rewriter_config_builder::{content_handlers, ContentFunction};
use super::speedreader::*;
use super::whitelist::Whitelist;

pub struct SpeedReaderStreaming<'h, O>
where
    O: OutputSink,
{
    url: Url,
    rewriter: HtmlRewriter<'h, O>,
}

impl<'h, O: OutputSink> SpeedReaderProcessor for SpeedReaderStreaming<'h, O> {
    fn write(&mut self, chunk: &[u8]) -> Result<(), SpeedReaderError> {
        self.rewriter.write(chunk)?;
        Ok(())
    }

    fn end(&mut self) -> Result<(), SpeedReaderError> {
        self.rewriter.end()?;
        Ok(())
    }

    fn rewriter_type(&self) -> RewriterType {
        RewriterType::Streaming
    }
}

impl<'h, O: OutputSink> SpeedReaderStreaming<'h, O> {
    pub fn try_new(
        url: Url,
        output_sink: O,
        config: &'h [(Selector, ContentFunction)],
    ) -> Result<Self, SpeedReaderError> {
        let mut whitelist = Whitelist::default();
        whitelist.load_predefined();
        let rewriter = HtmlRewriter::try_new(
            Settings {
                element_content_handlers: content_handlers(config),
                document_content_handlers: vec![doc_comments!(|el| {
                    el.remove();
                    Ok(())
                })],
                ..Settings::default()
            },
            output_sink,
        )?;

        let sr = SpeedReaderStreaming { url, rewriter };

        Ok(sr)
    }
}
