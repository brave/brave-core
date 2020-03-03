use html5ever::driver::{ParseOpts, Parser};
use html5ever::rcdom::Handle;
use html5ever::rcdom::NodeData;
use html5ever::rcdom::RcDom;
use html5ever::tendril::*;
use html5ever::tree_builder::{AppendText, ElementFlags, NodeOrText, QuirksMode, TreeSink};
use html5ever::{Attribute, ExpandedName, QualName};
use std::borrow::Cow;
use std::clone::Clone;
use std::collections::HashMap;
use std::default::Default;
use std::string::String;
use std::vec::Vec;
use url::Url;

use crate::speedreader::SpeedReaderError;

// Feature extractor which accepts chunks of data to parse
pub struct FeatureExtractorStreamer {
    inner: Parser<FeaturisingTreeSink>,
}

impl FeatureExtractorStreamer {
    pub fn try_new(url: &Url) -> Result<Self, SpeedReaderError> {
        let mut sink = FeaturisingTreeSink::default();
        sink.features.insert(
            "url_depth".to_string(),
            url_depth(url).unwrap_or_default() as u32,
        );

        let parser = html5ever::parse_document(sink, ParseOpts::default());

        Ok(FeatureExtractorStreamer { inner: parser })
    }

    pub fn write(&mut self, fragment: &mut &[u8]) -> Result<(), ()> {
        let tend = StrTendril::try_from_byte_slice(fragment)?;
        self.inner.process(tend);
        Ok(())
    }

    pub fn end(&mut self) -> &mut FeaturisingTreeSink {
        &mut self.inner.tokenizer.sink.sink
    }

    pub fn features(&self) -> &HashMap<String, u32> {
        &self.inner.tokenizer.sink.sink.features
    }
}

fn url_depth(url: &Url) -> Result<usize, SpeedReaderError> {
    url.path_segments()
        .map(std::iter::Iterator::count) // want number of segments only
        .ok_or_else(|| SpeedReaderError::InvalidUrl(url.as_str().to_owned())) // return error
}

pub struct FeaturisingTreeSink {
    pub features: HashMap<String, u32>,
    pub rcdom: RcDom,
}

impl Clone for FeaturisingTreeSink {
    fn clone(&self) -> Self {
        let mut cloned_f = HashMap::new();
        for (k, v) in self.features.iter() {
            cloned_f.insert(k.to_string(), *v);
        }
        let cloned_r = RcDom {
            document: self.rcdom.document.clone(),
            errors: self.rcdom.errors.clone(),
            quirks_mode: self.rcdom.quirks_mode,
        };

        FeaturisingTreeSink {
            features: cloned_f,
            rcdom: cloned_r,
        }
    }
}

impl Default for FeaturisingTreeSink {
    fn default() -> FeaturisingTreeSink {
        FeaturisingTreeSink {
            features: HashMap::new(),
            rcdom: RcDom::default(),
        }
    }
}

impl TreeSink for FeaturisingTreeSink {
    type Output = Self;
    type Handle = Handle;
    fn finish(self) -> Self {
        self
    }

    // everytime the parser identifies a new element, our sink will figure out
    // if the element is part of the subset used by our classifier. if that is
    // the case, increases the respective feature counter.
    fn create_element(
        &mut self,
        name: QualName,
        attrs: Vec<Attribute>,
        flags: ElementFlags,
    ) -> Handle {
        // increases count on feature map for selected tags
        let elem = name.local.to_string();
        self.features
            .entry(elem.clone())
            .and_modify(|v| *v += 1)
            .or_insert(1);

        // seaches for `<meta property="{og:},{fb:}..." />`
        if elem == "meta" {
            for a in attrs.clone() {
                let attr = a.value.to_string();

                if attr.starts_with("og:") {
                    self.features
                        .entry("og_article".to_string())
                        .and_modify(|v| *v = 1)
                        .or_insert(1);
                }

                if attr.starts_with("fb:") {
                    self.features
                        .entry("fb_pages".to_string())
                        .and_modify(|v| *v = 1)
                        .or_insert(1);
                }
            }
        }

        // checks if page is AMP compatible
        if elem == "link" {
            for a in attrs.clone() {
                if a.value.to_string() == "amphtml" {
                    self.features
                        .entry("amphtml".to_string())
                        .and_modify(|v| *v = 1)
                        .or_insert(1);
                }
            }
        }

        // checks if element has namespace `ns:schema.org:Article` or `ns:schema.org:NewsArticle`
        for a in attrs.clone() {
            if a.value
                .to_string()
                .starts_with("https://schema.org/Article")
                || a.value
                    .to_string()
                    .starts_with("https://schema.org/NewsArticle")
            {
                self.features
                    .entry("schema_org".to_string())
                    .and_modify(|v| *v = 1)
                    .or_insert(1);
            }
        }

        self.rcdom.create_element(name, attrs, flags)
    }

    // everytime the append node is required from our sink, it will either
    // 1) update the tag level tracker by setting the new node's level as +1
    //    from its parent and
    // 2) if the node to append is of type `text`, it will proceed to calculate
    //    the number of words in the text node and add that information to the
    //    feature list. It will also decide whether the text is a `text_block`
    //    and update the feature list accordingly.
    fn append(&mut self, parent: &Handle, child: NodeOrText<Handle>) {
        if let AppendText(text) = &child {
            if let NodeData::Element { name, .. } = &parent.data {
                let parent_name = name.local.to_string();

                if parent_name == "p" {
                    let parent_level = node_depth(parent, 11, 1);
                    let num_words = text.split_whitespace().count();

                    // words
                    self.features
                        .entry("words".to_string())
                        .and_modify(|v| *v += num_words as u32)
                        .or_insert(num_words as u32);

                    let text_blocks = self.features.entry("text_blocks".to_string()).or_insert(0);

                    // text_blocks
                    if num_words > 400 && parent_level > 1 && parent_level < 11 {
                        *text_blocks += 1;
                    }
                }
            }
        }

        self.rcdom.append(parent, child)
    }

    // Default TreeSink meethods from rcdom

    fn parse_error(&mut self, msg: Cow<'static, str>) {
        self.rcdom.parse_error(msg);
    }

    fn get_document(&mut self) -> Handle {
        self.rcdom.get_document()
    }

    fn get_template_contents(&mut self, target: &Handle) -> Handle {
        self.rcdom.get_template_contents(target)
    }

    fn set_quirks_mode(&mut self, mode: QuirksMode) {
        self.rcdom.set_quirks_mode(mode)
    }

    fn same_node(&self, x: &Handle, y: &Handle) -> bool {
        self.rcdom.same_node(x, y)
    }

    fn elem_name<'a>(&'a self, target: &'a Handle) -> ExpandedName<'a> {
        self.rcdom.elem_name(target)
    }

    fn create_comment(&mut self, text: StrTendril) -> Handle {
        self.rcdom.create_comment(text)
    }

    fn create_pi(&mut self, target: StrTendril, content: StrTendril) -> Handle {
        self.rcdom.create_pi(target, content)
    }

    fn append_before_sibling(&mut self, sibling: &Handle, child: NodeOrText<Handle>) {
        self.rcdom.append_before_sibling(sibling, child)
    }

    fn append_based_on_parent_node(
        &mut self,
        element: &Handle,
        prev_element: &Handle,
        child: NodeOrText<Handle>,
    ) {
        self.rcdom
            .append_based_on_parent_node(element, prev_element, child)
    }

    fn append_doctype_to_document(
        &mut self,
        name: StrTendril,
        public_id: StrTendril,
        system_id: StrTendril,
    ) {
        self.rcdom
            .append_doctype_to_document(name, public_id, system_id);
    }

    fn add_attrs_if_missing(&mut self, target: &Handle, attrs: Vec<Attribute>) {
        self.rcdom.add_attrs_if_missing(target, attrs);
    }

    fn remove_from_parent(&mut self, target: &Handle) {
        self.rcdom.remove_from_parent(target);
    }

    fn reparent_children(&mut self, node: &Handle, new_parent: &Handle) {
        self.rcdom.reparent_children(node, new_parent);
    }

    fn mark_script_already_started(&mut self, target: &Handle) {
        self.rcdom.mark_script_already_started(target);
    }
}

fn node_depth(node: &Handle, max_depth: usize, current_depth: usize) -> usize {
    if current_depth > max_depth {
        return current_depth;
    }
    if let Some(parent) = node.parent.take() {
        if let Some(strong_parent) = parent.upgrade() {
            node_depth(&strong_parent, max_depth, current_depth + 1)
        } else {
            current_depth
        }
    } else {
        current_depth
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_depth_url() {
        assert_eq!(url_depth(&Url::parse("http://url.com").unwrap()), Ok(1));
        assert_eq!(url_depth(&Url::parse("http://url.com/").unwrap()), Ok(1));

        assert_eq!(
            url_depth(&Url::parse("http://url.com/another/path/here?test").unwrap()),
            Ok(3)
        );

        assert_eq!(
            url_depth(&Url::parse("https://www.url.com/another/path").unwrap()),
            Ok(2)
        );
        assert!(matches!(
            url_depth(&Url::parse("data:text/plain,HelloWorld").unwrap()),
            Err(SpeedReaderError::InvalidUrl(_))
        ));
    }
}
