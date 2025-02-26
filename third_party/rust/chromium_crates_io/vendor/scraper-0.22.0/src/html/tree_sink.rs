use super::Html;
use crate::node::{Comment, Doctype, Element, Node, ProcessingInstruction, Text};
use crate::tendril_util::make as make_tendril;
use ego_tree::NodeId;
use html5ever::tendril::StrTendril;
use html5ever::tree_builder::{ElementFlags, NodeOrText, QuirksMode, TreeSink};
use html5ever::Attribute;
use html5ever::QualName;
use std::borrow::Cow;
use std::cell::{Ref, RefCell};

/// Wraps `Html` instances as sinks to drive parsing
#[derive(Debug)]
pub struct HtmlTreeSink(RefCell<Html>);

impl HtmlTreeSink {
    /// Wrap a `Html`instance as a sink to drive parsing
    pub fn new(html: Html) -> Self {
        Self(RefCell::new(html))
    }
}

/// Note: does not support the `<template>` element.
impl TreeSink for HtmlTreeSink {
    type Output = Html;
    type Handle = NodeId;
    type ElemName<'a> = Ref<'a, QualName>;

    fn finish(self) -> Html {
        self.0.into_inner()
    }

    // Signal a parse error.
    fn parse_error(&self, msg: Cow<'static, str>) {
        #[cfg(feature = "errors")]
        self.0.borrow_mut().errors.push(msg);
        #[cfg(not(feature = "errors"))]
        let _ = msg;
    }

    // Set the document's quirks mode.
    fn set_quirks_mode(&self, mode: QuirksMode) {
        self.0.borrow_mut().quirks_mode = mode;
    }

    // Get a handle to the Document node.
    fn get_document(&self) -> Self::Handle {
        self.0.borrow().tree.root().id()
    }

    // Do two handles refer to the same node?
    fn same_node(&self, x: &Self::Handle, y: &Self::Handle) -> bool {
        x == y
    }

    // What is the name of this element?
    //
    // Should never be called on a non-element node; feel free to panic!.
    fn elem_name<'a>(&'a self, target: &Self::Handle) -> Ref<'a, QualName> {
        Ref::map(self.0.borrow(), |this| {
            &this
                .tree
                .get(*target)
                .unwrap()
                .value()
                .as_element()
                .unwrap()
                .name
        })
    }

    // Create an element.
    //
    // When creating a template element (name.expanded() == expanded_name!(html "template")), an
    // associated document fragment called the "template contents" should also be created. Later
    // calls to self.get_template_contents() with that given element return it.
    fn create_element(
        &self,
        name: QualName,
        attrs: Vec<Attribute>,
        _flags: ElementFlags,
    ) -> Self::Handle {
        let fragment = name.expanded() == expanded_name!(html "template");

        let mut this = self.0.borrow_mut();
        let mut node = this.tree.orphan(Node::Element(Element::new(name, attrs)));

        if fragment {
            node.append(Node::Fragment);
        }

        node.id()
    }

    // Create a comment node.
    fn create_comment(&self, text: StrTendril) -> Self::Handle {
        self.0
            .borrow_mut()
            .tree
            .orphan(Node::Comment(Comment {
                comment: make_tendril(text),
            }))
            .id()
    }

    // Append a DOCTYPE element to the Document node.
    fn append_doctype_to_document(
        &self,
        name: StrTendril,
        public_id: StrTendril,
        system_id: StrTendril,
    ) {
        let name = make_tendril(name);
        let public_id = make_tendril(public_id);
        let system_id = make_tendril(system_id);
        let doctype = Doctype {
            name,
            public_id,
            system_id,
        };
        self.0
            .borrow_mut()
            .tree
            .root_mut()
            .append(Node::Doctype(doctype));
    }

    // Append a node as the last child of the given node. If this would produce adjacent sibling
    // text nodes, it should concatenate the text instead.
    //
    // The child node will not already have a parent.
    fn append(&self, parent: &Self::Handle, child: NodeOrText<Self::Handle>) {
        let mut this = self.0.borrow_mut();
        let mut parent = this.tree.get_mut(*parent).unwrap();

        match child {
            NodeOrText::AppendNode(id) => {
                parent.append_id(id);
            }

            NodeOrText::AppendText(text) => {
                let text = make_tendril(text);

                let did_concat = parent.last_child().is_some_and(|mut n| match n.value() {
                    Node::Text(t) => {
                        t.text.push_tendril(&text);
                        true
                    }
                    _ => false,
                });

                if !did_concat {
                    parent.append(Node::Text(Text { text }));
                }
            }
        }
    }

    // Append a node as the sibling immediately before the given node. If that node has no parent,
    // do nothing and return Err(new_node).
    //
    // The tree builder promises that sibling is not a text node. However its old previous sibling,
    // which would become the new node's previous sibling, could be a text node. If the new node is
    // also a text node, the two should be merged, as in the behavior of append.
    //
    // NB: new_node may have an old parent, from which it should be removed.
    fn append_before_sibling(&self, sibling: &Self::Handle, new_node: NodeOrText<Self::Handle>) {
        let mut this = self.0.borrow_mut();

        if let NodeOrText::AppendNode(id) = new_node {
            this.tree.get_mut(id).unwrap().detach();
        }

        let mut sibling = this.tree.get_mut(*sibling).unwrap();
        if sibling.parent().is_some() {
            match new_node {
                NodeOrText::AppendNode(id) => {
                    sibling.insert_id_before(id);
                }

                NodeOrText::AppendText(text) => {
                    let text = make_tendril(text);

                    let did_concat = sibling.prev_sibling().is_some_and(|mut n| match n.value() {
                        Node::Text(t) => {
                            t.text.push_tendril(&text);
                            true
                        }
                        _ => false,
                    });

                    if !did_concat {
                        sibling.insert_before(Node::Text(Text { text }));
                    }
                }
            }
        }
    }

    // Detach the given node from its parent.
    fn remove_from_parent(&self, target: &Self::Handle) {
        self.0.borrow_mut().tree.get_mut(*target).unwrap().detach();
    }

    // Remove all the children from node and append them to new_parent.
    fn reparent_children(&self, node: &Self::Handle, new_parent: &Self::Handle) {
        self.0
            .borrow_mut()
            .tree
            .get_mut(*new_parent)
            .unwrap()
            .reparent_from_id_append(*node);
    }

    // Add each attribute to the given element, if no attribute with that name already exists. The
    // tree builder promises this will never be called with something else than an element.
    fn add_attrs_if_missing(&self, target: &Self::Handle, attrs: Vec<Attribute>) {
        let mut this = self.0.borrow_mut();
        let mut node = this.tree.get_mut(*target).unwrap();
        let element = match *node.value() {
            Node::Element(ref mut e) => e,
            _ => unreachable!(),
        };

        for attr in attrs {
            #[cfg(not(feature = "deterministic"))]
            if let Err(idx) = element
                .attrs
                .binary_search_by(|(name, _)| name.cmp(&attr.name))
            {
                element
                    .attrs
                    .insert(idx, (attr.name, make_tendril(attr.value)));
            }

            #[cfg(feature = "deterministic")]
            element
                .attrs
                .entry(attr.name)
                .or_insert_with(|| make_tendril(attr.value));
        }
    }

    // Get a handle to a template's template contents.
    //
    // The tree builder promises this will never be called with something else than a template
    // element.
    fn get_template_contents(&self, target: &Self::Handle) -> Self::Handle {
        self.0
            .borrow()
            .tree
            .get(*target)
            .unwrap()
            .first_child()
            .unwrap()
            .id()
    }

    // Mark a HTML <script> element as "already started".
    fn mark_script_already_started(&self, _node: &Self::Handle) {}

    // Create Processing Instruction.
    fn create_pi(&self, target: StrTendril, data: StrTendril) -> Self::Handle {
        let target = make_tendril(target);
        let data = make_tendril(data);
        self.0
            .borrow_mut()
            .tree
            .orphan(Node::ProcessingInstruction(ProcessingInstruction {
                target,
                data,
            }))
            .id()
    }

    fn append_based_on_parent_node(
        &self,
        element: &Self::Handle,
        prev_element: &Self::Handle,
        child: NodeOrText<Self::Handle>,
    ) {
        let has_parent = self
            .0
            .borrow()
            .tree
            .get(*element)
            .unwrap()
            .parent()
            .is_some();

        if has_parent {
            self.append_before_sibling(element, child)
        } else {
            self.append(prev_element, child)
        }
    }
}
