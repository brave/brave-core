use html5ever::tendril::StrTendril;
use html5ever::tree_builder::{ElementFlags, NodeOrText, QuirksMode, TreeSink};
use html5ever::{self, Attribute, ExpandedName, QualName};
use std::borrow::Cow;

use crate::attributes;
use crate::tree::NodeRef;

/// Options for the HTML parser.
#[derive(Default)]
pub struct ParseOpts {
    /// Options for the HTML tokenizer.
    pub tokenizer: html5ever::tokenizer::TokenizerOpts,

    /// Options for the HTML tree builder.
    pub tree_builder: html5ever::tree_builder::TreeBuilderOpts,

    /// A callback for HTML parse errors (which are never fatal).
    pub on_parse_error: Option<Box<dyn FnMut(Cow<'static, str>)>>,
}

/// Parse an HTML document with html5ever and the default configuration.
pub fn parse_html() -> html5ever::Parser<Sink> {
    parse_html_with_options(ParseOpts::default())
}

/// Parse an HTML document with html5ever with custom configuration.
pub fn parse_html_with_options(opts: ParseOpts) -> html5ever::Parser<Sink> {
    let sink = Sink {
        document_node: NodeRef::new_document(),
        on_parse_error: opts.on_parse_error,
    };
    let html5opts = html5ever::ParseOpts {
        tokenizer: opts.tokenizer,
        tree_builder: opts.tree_builder,
    };
    html5ever::parse_document(sink, html5opts)
}

/// Parse an HTML fragment with html5ever and the default configuration.
pub fn parse_fragment(ctx_name: QualName, ctx_attr: Vec<Attribute>) -> html5ever::Parser<Sink> {
    parse_fragment_with_options(ParseOpts::default(), ctx_name, ctx_attr)
}

/// Parse an HTML fragment with html5ever with custom configuration.
pub fn parse_fragment_with_options(
    opts: ParseOpts,
    ctx_name: QualName,
    ctx_attr: Vec<Attribute>,
) -> html5ever::Parser<Sink> {
    let sink = Sink {
        document_node: NodeRef::new_document(),
        on_parse_error: opts.on_parse_error,
    };
    let html5opts = html5ever::ParseOpts {
        tokenizer: opts.tokenizer,
        tree_builder: opts.tree_builder,
    };
    html5ever::parse_fragment(sink, html5opts, ctx_name, ctx_attr)
}

/// Receives new tree nodes during parsing.
pub struct Sink {
    /// The `Document` itself.
    pub document_node: NodeRef,

    /// The Sink will invoke this callback if it encounters a parse error.
    pub on_parse_error: Option<Box<dyn FnMut(Cow<'static, str>)>>,
}

impl Default for Sink {
    fn default() -> Sink {
        Sink {
            document_node: NodeRef::new_document(),
            on_parse_error: None,
        }
    }
}

impl TreeSink for Sink {
    type Output = Self;

    fn finish(self) -> Self {
        self
    }

    type Handle = NodeRef;

    #[inline]
    fn parse_error(&mut self, message: Cow<'static, str>) {
        if let Some(ref mut handler) = self.on_parse_error {
            handler(message)
        }
    }

    #[inline]
    fn get_document(&mut self) -> NodeRef {
        self.document_node.clone()
    }

    #[inline]
    fn set_quirks_mode(&mut self, mode: QuirksMode) {
        self.document_node
            .as_document()
            .unwrap()
            ._quirks_mode
            .set(mode)
    }

    #[inline]
    fn same_node(&self, x: &NodeRef, y: &NodeRef) -> bool {
        x == y
    }

    #[inline]
    fn elem_name<'a>(&self, target: &'a NodeRef) -> ExpandedName<'a> {
        target.as_element().unwrap().name.expanded()
    }

    #[inline]
    fn create_element(
        &mut self,
        name: QualName,
        attrs: Vec<Attribute>,
        _flags: ElementFlags,
    ) -> NodeRef {
        NodeRef::new_element(
            name,
            attrs.into_iter().map(|attr| {
                let Attribute {
                    name: QualName { prefix, ns, local },
                    value,
                } = attr;
                let value = String::from(value);
                (
                    attributes::ExpandedName { ns, local },
                    attributes::Attribute { prefix, value },
                )
            }),
        )
    }

    #[inline]
    fn create_comment(&mut self, text: StrTendril) -> NodeRef {
        NodeRef::new_comment(text)
    }

    #[inline]
    fn create_pi(&mut self, target: StrTendril, data: StrTendril) -> NodeRef {
        NodeRef::new_processing_instruction(target, data)
    }

    #[inline]
    fn append(&mut self, parent: &NodeRef, child: NodeOrText<NodeRef>) {
        match child {
            NodeOrText::AppendNode(node) => parent.append(node),
            NodeOrText::AppendText(text) => {
                if let Some(last_child) = parent.last_child() {
                    if let Some(existing) = last_child.as_text() {
                        existing.borrow_mut().push_str(&text);
                        return;
                    }
                }
                parent.append(NodeRef::new_text(text))
            }
        }
    }

    #[inline]
    fn append_before_sibling(&mut self, sibling: &NodeRef, child: NodeOrText<NodeRef>) {
        match child {
            NodeOrText::AppendNode(node) => sibling.insert_before(node),
            NodeOrText::AppendText(text) => {
                if let Some(previous_sibling) = sibling.previous_sibling() {
                    if let Some(existing) = previous_sibling.as_text() {
                        existing.borrow_mut().push_str(&text);
                        return;
                    }
                }
                sibling.insert_before(NodeRef::new_text(text))
            }
        }
    }

    #[inline]
    fn append_doctype_to_document(
        &mut self,
        name: StrTendril,
        public_id: StrTendril,
        system_id: StrTendril,
    ) {
        self.document_node
            .append(NodeRef::new_doctype(name, public_id, system_id))
    }

    #[inline]
    fn add_attrs_if_missing(&mut self, target: &NodeRef, attrs: Vec<Attribute>) {
        let element = target.as_element().unwrap();
        let mut attributes = element.attributes.borrow_mut();

        for Attribute {
            name: QualName { prefix, ns, local },
            value,
        } in attrs
        {
            attributes
                .map
                .entry(attributes::ExpandedName { ns, local })
                .or_insert_with(|| {
                    let value = String::from(value);
                    attributes::Attribute { prefix, value }
                });
        }
    }

    #[inline]
    fn remove_from_parent(&mut self, target: &NodeRef) {
        target.detach()
    }

    #[inline]
    fn reparent_children(&mut self, node: &NodeRef, new_parent: &NodeRef) {
        // FIXME: Can this be done more effciently in rctree,
        // by moving the whole linked list of children at once?
        for child in node.children() {
            new_parent.append(child)
        }
    }

    #[inline]
    fn mark_script_already_started(&mut self, _node: &NodeRef) {
        // FIXME: Is this useful outside of a browser?
    }

    #[inline]
    fn get_template_contents(&mut self, target: &NodeRef) -> NodeRef {
        target
            .as_element()
            .unwrap()
            .template_contents
            .clone()
            .unwrap()
    }

    fn append_based_on_parent_node(
        &mut self,
        element: &NodeRef,
        prev_element: &NodeRef,
        child: NodeOrText<NodeRef>,
    ) {
        if element.parent().is_some() {
            self.append_before_sibling(element, child)
        } else {
            self.append(prev_element, child)
        }
    }
}
