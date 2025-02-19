use crate::tree::{Doctype, DocumentData, ElementData, Node, NodeRef};
use std::cell::RefCell;
use std::fmt;
use std::ops::Deref;

impl NodeRef {
    /// If this node is an element, return a strong reference to element-specific data.
    #[inline]
    pub fn into_element_ref(self) -> Option<NodeDataRef<ElementData>> {
        NodeDataRef::new_opt(self, Node::as_element)
    }

    /// If this node is a text node, return a strong reference to its contents.
    #[inline]
    pub fn into_text_ref(self) -> Option<NodeDataRef<RefCell<String>>> {
        NodeDataRef::new_opt(self, Node::as_text)
    }

    /// If this node is a comment, return a strong reference to its contents.
    #[inline]
    pub fn into_comment_ref(self) -> Option<NodeDataRef<RefCell<String>>> {
        NodeDataRef::new_opt(self, Node::as_comment)
    }

    /// If this node is a doctype, return a strong reference to doctype-specific data.
    #[inline]
    pub fn into_doctype_ref(self) -> Option<NodeDataRef<Doctype>> {
        NodeDataRef::new_opt(self, Node::as_doctype)
    }

    /// If this node is a document, return a strong reference to document-specific data.
    #[inline]
    pub fn into_document_ref(self) -> Option<NodeDataRef<DocumentData>> {
        NodeDataRef::new_opt(self, Node::as_document)
    }
}

/// Holds a strong reference to a node, but dereferences to some component inside of it.
#[derive(Eq)]
pub struct NodeDataRef<T> {
    _keep_alive: NodeRef,
    _reference: *const T,
}

impl<T> NodeDataRef<T> {
    /// Create a `NodeDataRef` for a component in a given node.
    #[inline]
    pub fn new<F>(rc: NodeRef, f: F) -> NodeDataRef<T>
    where
        F: FnOnce(&Node) -> &T,
    {
        NodeDataRef {
            _reference: f(&rc),
            _keep_alive: rc,
        }
    }

    /// Create a `NodeDataRef` for and a component that may or may not be in a given node.
    #[inline]
    pub fn new_opt<F>(rc: NodeRef, f: F) -> Option<NodeDataRef<T>>
    where
        F: FnOnce(&Node) -> Option<&T>,
    {
        f(&rc).map(|r| r as *const T).map(move |r| NodeDataRef {
            _reference: r,
            _keep_alive: rc,
        })
    }

    /// Access the corresponding node.
    #[inline]
    pub fn as_node(&self) -> &NodeRef {
        &self._keep_alive
    }
}

impl<T> Deref for NodeDataRef<T> {
    type Target = T;
    #[inline]
    fn deref(&self) -> &T {
        unsafe { &*self._reference }
    }
}

// #[derive(PartialEq)] would compare both fields
impl<T> PartialEq for NodeDataRef<T> {
    #[inline]
    fn eq(&self, other: &Self) -> bool {
        self._keep_alive == other._keep_alive
    }
}

// #[derive(Clone)] would have an unnecessary `T: Clone` bound
impl<T> Clone for NodeDataRef<T> {
    #[inline]
    fn clone(&self) -> Self {
        NodeDataRef {
            _keep_alive: self._keep_alive.clone(),
            _reference: self._reference,
        }
    }
}

impl<T: fmt::Debug> fmt::Debug for NodeDataRef<T> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        fmt::Debug::fmt(&**self, f)
    }
}

impl NodeDataRef<ElementData> {
    /// Return the concatenation of all text nodes in this subtree.
    pub fn text_contents(&self) -> String {
        self.as_node().text_contents()
    }
}
