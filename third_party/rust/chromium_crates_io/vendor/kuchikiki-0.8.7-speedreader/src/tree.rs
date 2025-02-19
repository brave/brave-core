use html5ever::tree_builder::QuirksMode;
use html5ever::QualName;
use std::cell::{Cell, RefCell};
use std::fmt;
use std::ops::Deref;
use std::rc::{Rc, Weak};

use crate::attributes::{Attribute, Attributes, ExpandedName};
use crate::cell_extras::*;
use crate::iter::NodeIterator;

/// Node data specific to the node type.
#[derive(Debug, PartialEq, Clone)]
pub enum NodeData {
    /// Element node
    Element(ElementData),

    /// Text node
    Text(RefCell<String>),

    /// Comment node
    Comment(RefCell<String>),

    /// Processing instruction node
    ProcessingInstruction(RefCell<(String, String)>),

    /// Doctype node
    Doctype(Doctype),

    /// Document node
    Document(DocumentData),

    /// Document fragment node
    DocumentFragment,
}

/// Data specific to doctype nodes.
#[derive(Debug, PartialEq, Clone)]
pub struct Doctype {
    /// The name of the doctype
    pub name: String,

    /// The public ID of the doctype
    pub public_id: String,

    /// The system ID of the doctype
    pub system_id: String,
}

/// Data specific to element nodes.
#[derive(Debug, PartialEq, Clone)]
pub struct ElementData {
    /// The namespace and local name of the element, such as `ns!(html)` and `body`.
    pub name: QualName,

    /// The attributes of the elements.
    pub attributes: RefCell<Attributes>,

    /// If the element is an HTML `<template>` element,
    /// the document fragment node that is the root of template contents.
    pub template_contents: Option<NodeRef>,

    /// The element's score for whether it should be considered for the
    /// "readable" tree.
    pub score: Cell<f32>,

    /// The element's score for whether it should be considered for the
    /// "readable" tree.
    pub is_candidate: Cell<bool>,
}

/// Data specific to document nodes.
#[derive(Debug, PartialEq, Clone)]
pub struct DocumentData {
    #[doc(hidden)]
    pub _quirks_mode: Cell<QuirksMode>,
}

impl DocumentData {
    /// The quirks mode of the document, as determined by the HTML parser.
    #[inline]
    pub fn quirks_mode(&self) -> QuirksMode {
        self._quirks_mode.get()
    }
}

/// A strong reference to a node.
///
/// A node is destroyed when the last strong reference to it dropped.
///
/// Each node holds a strong reference to its first child and next sibling (if any),
/// but only a weak reference to its last child, previous sibling, and parent.
/// This is to avoid strong reference cycles, which would cause memory leaks.
///
/// As a result, a single `NodeRef` is sufficient to keep alive a node
/// and nodes that are after it in tree order
/// (its descendants, its following siblings, and their descendants)
/// but not other nodes in a tree.
///
/// To avoid detroying nodes prematurely,
/// programs typically hold a strong reference to the root of a document
/// until they’re done with that document.
#[derive(Clone, Debug)]
pub struct NodeRef(pub Rc<Node>);

impl Deref for NodeRef {
    type Target = Node;
    #[inline]
    fn deref(&self) -> &Node {
        &self.0
    }
}

impl Eq for NodeRef {}
impl PartialEq for NodeRef {
    #[inline]
    fn eq(&self, other: &NodeRef) -> bool {
        let a: *const Node = &*self.0;
        let b: *const Node = &*other.0;
        a == b
    }
}

/// A node inside a DOM-like tree.
pub struct Node {
    parent: Cell<Option<Weak<Node>>>,
    previous_sibling: Cell<Option<Weak<Node>>>,
    next_sibling: Cell<Option<Rc<Node>>>,
    first_child: Cell<Option<Rc<Node>>>,
    last_child: Cell<Option<Weak<Node>>>,
    data: NodeData,
}

impl fmt::Debug for Node {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "{:?} @ {:?}", self.data, self as *const Node)
    }
}

/// Prevent implicit recursion when dropping nodes to avoid overflowing the stack.
///
/// The implicit drop is correct, but recursive.
/// In the worst case (where no node has both a next sibling and a child),
/// a tree of a few tens of thousands of nodes could cause a stack overflow.
///
/// This `Drop` implementations makes sure the recursion does not happen.
/// Instead, it has an explicit `Vec<Rc<Node>>` stack to traverse the subtree,
/// but only following `Rc<Node>` references that are "unique":
/// that have a strong reference count of 1.
/// Those are the nodes that would have been dropped recursively.
///
/// The stack holds ancestors of the current node rather than preceding siblings,
/// on the assumption that large document trees are typically wider than deep.
impl Drop for Node {
    fn drop(&mut self) {
        // `.take_if_unique_strong()` temporarily leaves the tree in an inconsistent state,
        // as the corresponding `Weak` reference in the other direction is not removed.
        // It is important that all `Some(_)` strong references it returns
        // are dropped by the end of this `drop` call,
        // and that no user code is invoked in-between.

        // Sharing `stack` between these two calls is not necessary,
        // but it allows re-using memory allocations.
        let mut stack = Vec::new();
        if let Some(rc) = self.first_child.take_if_unique_strong() {
            non_recursive_drop_unique_rc(rc, &mut stack);
        }
        if let Some(rc) = self.next_sibling.take_if_unique_strong() {
            non_recursive_drop_unique_rc(rc, &mut stack);
        }

        fn non_recursive_drop_unique_rc(mut rc: Rc<Node>, stack: &mut Vec<Rc<Node>>) {
            loop {
                if let Some(child) = rc.first_child.take_if_unique_strong() {
                    stack.push(rc);
                    rc = child;
                    continue;
                }
                if let Some(sibling) = rc.next_sibling.take_if_unique_strong() {
                    // The previous value of `rc: Rc<Node>` is dropped here.
                    // Since it was unique, the corresponding `Node` is dropped as well.
                    // `<Node as Drop>::drop` does not call `drop_rc`
                    // as both the first child and next sibling were already taken.
                    // Weak reference counts decremented here for `Cell`s that are `Some`:
                    // * `rc.parent`: still has a strong reference in `stack` or elsewhere
                    // * `rc.last_child`: this is the last weak ref. Deallocated now.
                    // * `rc.previous_sibling`: this is the last weak ref. Deallocated now.
                    rc = sibling;
                    continue;
                }
                if let Some(parent) = stack.pop() {
                    // Same as in the above comment.
                    rc = parent;
                    continue;
                }
                return;
            }
        }
    }
}

impl NodeRef {
    /// Create a new node.
    #[inline]
    pub fn new(data: NodeData) -> NodeRef {
        NodeRef(Rc::new(Node {
            parent: Cell::new(None),
            first_child: Cell::new(None),
            last_child: Cell::new(None),
            previous_sibling: Cell::new(None),
            next_sibling: Cell::new(None),
            data,
        }))
    }

    /// Create a new element node.
    #[inline]
    pub fn new_element<I>(name: QualName, attributes: I) -> NodeRef
    where
        I: IntoIterator<Item = (ExpandedName, Attribute)>,
    {
        NodeRef::new(NodeData::Element(ElementData {
            template_contents: if name.expanded() == expanded_name!(html "template") {
                Some(NodeRef::new(NodeData::DocumentFragment))
            } else {
                None
            },
            name,
            attributes: RefCell::new(Attributes {
                map: attributes.into_iter().collect(),
            }),
            score: Cell::new(0.0),
            is_candidate: Cell::new(false),
        }))
    }

    /// Create a new text node.
    #[inline]
    pub fn new_text<T: Into<String>>(value: T) -> NodeRef {
        NodeRef::new(NodeData::Text(RefCell::new(value.into())))
    }

    /// Create a new comment node.
    #[inline]
    pub fn new_comment<T: Into<String>>(value: T) -> NodeRef {
        NodeRef::new(NodeData::Comment(RefCell::new(value.into())))
    }

    /// Create a new processing instruction node.
    #[inline]
    pub fn new_processing_instruction<T1, T2>(target: T1, data: T2) -> NodeRef
    where
        T1: Into<String>,
        T2: Into<String>,
    {
        NodeRef::new(NodeData::ProcessingInstruction(RefCell::new((
            target.into(),
            data.into(),
        ))))
    }

    /// Create a new doctype node.
    #[inline]
    pub fn new_doctype<T1, T2, T3>(name: T1, public_id: T2, system_id: T3) -> NodeRef
    where
        T1: Into<String>,
        T2: Into<String>,
        T3: Into<String>,
    {
        NodeRef::new(NodeData::Doctype(Doctype {
            name: name.into(),
            public_id: public_id.into(),
            system_id: system_id.into(),
        }))
    }

    /// Create a new document node.
    #[inline]
    pub fn new_document() -> NodeRef {
        NodeRef::new(NodeData::Document(DocumentData {
            _quirks_mode: Cell::new(QuirksMode::NoQuirks),
        }))
    }

    /// Return the concatenation of all text nodes in this subtree.
    pub fn text_contents(&self) -> String {
        let mut s = String::new();
        for text_node in self.inclusive_descendants().text_nodes() {
            s.push_str(&text_node.borrow());
        }
        s
    }
}

impl Node {
    /// Return a reference to this node’s node-type-specific data.
    #[inline]
    pub fn data(&self) -> &NodeData {
        &self.data
    }

    /// If this node is an element, return a reference to element-specific data.
    #[inline]
    pub fn as_element(&self) -> Option<&ElementData> {
        match self.data {
            NodeData::Element(ref value) => Some(value),
            _ => None,
        }
    }

    /// If this node is a text node, return a reference to its contents.
    #[inline]
    pub fn as_text(&self) -> Option<&RefCell<String>> {
        match self.data {
            NodeData::Text(ref value) => Some(value),
            _ => None,
        }
    }

    /// If this node is a comment, return a reference to its contents.
    #[inline]
    pub fn as_comment(&self) -> Option<&RefCell<String>> {
        match self.data {
            NodeData::Comment(ref value) => Some(value),
            _ => None,
        }
    }

    /// If this node is a document, return a reference to doctype-specific data.
    #[inline]
    pub fn as_doctype(&self) -> Option<&Doctype> {
        match self.data {
            NodeData::Doctype(ref value) => Some(value),
            _ => None,
        }
    }

    /// If this node is a document, return a reference to document-specific data.
    #[inline]
    pub fn as_document(&self) -> Option<&DocumentData> {
        match self.data {
            NodeData::Document(ref value) => Some(value),
            _ => None,
        }
    }

    /// Return a reference to the parent node, unless this node is the root of the tree.
    #[inline]
    pub fn parent(&self) -> Option<NodeRef> {
        self.parent.upgrade().map(NodeRef)
    }

    /// Return a reference to the first child of this node, unless it has no child.
    #[inline]
    pub fn first_child(&self) -> Option<NodeRef> {
        self.first_child.clone_inner().map(NodeRef)
    }

    /// Return a reference to the last child of this node, unless it has no child.
    #[inline]
    pub fn last_child(&self) -> Option<NodeRef> {
        self.last_child.upgrade().map(NodeRef)
    }

    /// Return a reference to the previous sibling of this node, unless it is a first child.
    #[inline]
    pub fn previous_sibling(&self) -> Option<NodeRef> {
        self.previous_sibling.upgrade().map(NodeRef)
    }

    /// Return a reference to the next sibling of this node, unless it is a last child.
    #[inline]
    pub fn next_sibling(&self) -> Option<NodeRef> {
        self.next_sibling.clone_inner().map(NodeRef)
    }

    /// Detach a node from its parent and siblings. Children are not affected.
    ///
    /// To remove a node and its descendants, detach it and drop any strong reference to it.
    pub fn detach(&self) {
        let parent_weak = self.parent.take();
        let previous_sibling_weak = self.previous_sibling.take();
        let next_sibling_strong = self.next_sibling.take();

        let previous_sibling_opt = previous_sibling_weak
            .as_ref()
            .and_then(|weak| weak.upgrade());

        if let Some(next_sibling_ref) = next_sibling_strong.as_ref() {
            next_sibling_ref
                .previous_sibling
                .replace(previous_sibling_weak);
        } else if let Some(parent_ref) = parent_weak.as_ref() {
            if let Some(parent_strong) = parent_ref.upgrade() {
                parent_strong.last_child.replace(previous_sibling_weak);
            }
        }

        if let Some(previous_sibling_strong) = previous_sibling_opt {
            previous_sibling_strong
                .next_sibling
                .replace(next_sibling_strong);
        } else if let Some(parent_ref) = parent_weak.as_ref() {
            if let Some(parent_strong) = parent_ref.upgrade() {
                parent_strong.first_child.replace(next_sibling_strong);
            }
        }
    }
}

impl NodeRef {
    /// Append a new child to this node, after existing children.
    ///
    /// The new child is detached from its previous position.
    pub fn append(&self, new_child: NodeRef) {
        new_child.detach();
        new_child.parent.replace(Some(Rc::downgrade(&self.0)));
        if let Some(last_child_weak) = self.last_child.replace(Some(Rc::downgrade(&new_child.0))) {
            if let Some(last_child) = last_child_weak.upgrade() {
                new_child.previous_sibling.replace(Some(last_child_weak));
                debug_assert!(last_child.next_sibling.is_none());
                last_child.next_sibling.replace(Some(new_child.0));
                return;
            }
        }
        debug_assert!(self.first_child.is_none());
        self.first_child.replace(Some(new_child.0));
    }

    /// Prepend a new child to this node, before existing children.
    ///
    /// The new child is detached from its previous position.
    pub fn prepend(&self, new_child: NodeRef) {
        new_child.detach();
        new_child.parent.replace(Some(Rc::downgrade(&self.0)));
        if let Some(first_child) = self.first_child.take() {
            debug_assert!(first_child.previous_sibling.is_none());
            first_child
                .previous_sibling
                .replace(Some(Rc::downgrade(&new_child.0)));
            new_child.next_sibling.replace(Some(first_child));
        } else {
            debug_assert!(self.first_child.is_none());
            self.last_child.replace(Some(Rc::downgrade(&new_child.0)));
        }
        self.first_child.replace(Some(new_child.0));
    }

    /// Insert a new sibling after this node.
    ///
    /// The new sibling is detached from its previous position.
    pub fn insert_after(&self, new_sibling: NodeRef) {
        new_sibling.detach();
        new_sibling.parent.replace(self.parent.clone_inner());
        new_sibling
            .previous_sibling
            .replace(Some(Rc::downgrade(&self.0)));
        if let Some(next_sibling) = self.next_sibling.take() {
            debug_assert!(next_sibling.previous_sibling().unwrap() == *self);
            next_sibling
                .previous_sibling
                .replace(Some(Rc::downgrade(&new_sibling.0)));
            new_sibling.next_sibling.replace(Some(next_sibling));
        } else if let Some(parent) = self.parent() {
            debug_assert!(parent.last_child().unwrap() == *self);
            parent
                .last_child
                .replace(Some(Rc::downgrade(&new_sibling.0)));
        }
        self.next_sibling.replace(Some(new_sibling.0));
    }

    /// Insert a new sibling before this node.
    ///
    /// The new sibling is detached from its previous position.
    pub fn insert_before(&self, new_sibling: NodeRef) {
        new_sibling.detach();
        new_sibling.parent.replace(self.parent.clone_inner());
        new_sibling.next_sibling.replace(Some(self.0.clone()));
        if let Some(previous_sibling_weak) = self
            .previous_sibling
            .replace(Some(Rc::downgrade(&new_sibling.0)))
        {
            if let Some(previous_sibling) = previous_sibling_weak.upgrade() {
                new_sibling
                    .previous_sibling
                    .replace(Some(previous_sibling_weak));
                debug_assert!(previous_sibling.next_sibling().unwrap() == *self);
                previous_sibling.next_sibling.replace(Some(new_sibling.0));
                return;
            }
        }
        if let Some(parent) = self.parent() {
            debug_assert!(parent.first_child().unwrap() == *self);
            parent.first_child.replace(Some(new_sibling.0));
        }
    }
}
