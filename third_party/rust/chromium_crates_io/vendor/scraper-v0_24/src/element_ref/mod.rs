//! Element references.

use std::fmt::{self, Debug};
use std::iter::FusedIterator;
use std::ops::Deref;

use ego_tree::iter::{Edge, Traverse};
use ego_tree::NodeRef;
use html5ever::serialize::{serialize, SerializeOpts, TraversalScope};
use selectors::matching::SelectorCaches;

use crate::node::Element;
use crate::{Node, Selector};

/// Wrapper around a reference to an element node.
///
/// This wrapper implements the `Element` trait from the `selectors` crate, which allows it to be
/// matched against CSS selectors.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct ElementRef<'a> {
    node: NodeRef<'a, Node>,
}

impl<'a> ElementRef<'a> {
    fn new(node: NodeRef<'a, Node>) -> Self {
        ElementRef { node }
    }

    /// Wraps a `NodeRef` only if it references a `Node::Element`.
    pub fn wrap(node: NodeRef<'a, Node>) -> Option<Self> {
        if node.value().is_element() {
            Some(ElementRef::new(node))
        } else {
            None
        }
    }

    /// Returns the `Element` referenced by `self`.
    pub fn value(&self) -> &'a Element {
        self.node.value().as_element().unwrap()
    }

    /// Returns an iterator over descendent elements matching a selector.
    pub fn select<'b>(&self, selector: &'b Selector) -> Select<'a, 'b> {
        let mut inner = self.traverse();
        inner.next(); // Skip Edge::Open(self).

        Select {
            scope: *self,
            inner,
            selector,
            caches: Default::default(),
        }
    }

    fn serialize(&self, traversal_scope: TraversalScope) -> String {
        let opts = SerializeOpts {
            scripting_enabled: false, // It's not clear what this does.
            traversal_scope,
            create_missing_parent: false,
        };
        let mut buf = Vec::new();
        serialize(&mut buf, self, opts).unwrap();
        String::from_utf8(buf).unwrap()
    }

    /// Returns the HTML of this element.
    pub fn html(&self) -> String {
        self.serialize(TraversalScope::IncludeNode)
    }

    /// Returns the inner HTML of this element.
    pub fn inner_html(&self) -> String {
        self.serialize(TraversalScope::ChildrenOnly(None))
    }

    /// Returns the value of an attribute.
    pub fn attr(&self, attr: &str) -> Option<&'a str> {
        self.value().attr(attr)
    }

    /// Returns an iterator over descendent text nodes.
    pub fn text(&self) -> Text<'a> {
        Text {
            inner: self.traverse(),
        }
    }

    /// Iterate over all child nodes which are elements
    ///
    /// # Example
    ///
    /// ```
    /// # use scraper::Html;
    /// let fragment = Html::parse_fragment("foo<span>bar</span><a>baz</a>qux");
    ///
    /// let children = fragment.root_element().child_elements().map(|element| element.value().name()).collect::<Vec<_>>();
    /// assert_eq!(children, ["span", "a"]);
    /// ```
    pub fn child_elements(&self) -> impl Iterator<Item = ElementRef<'a>> {
        self.children().filter_map(ElementRef::wrap)
    }

    /// Iterate over all descendent nodes which are elements
    ///
    /// # Example
    ///
    /// ```
    /// # use scraper::Html;
    /// let fragment = Html::parse_fragment("foo<span><b>bar</b></span><a><i>baz</i></a>qux");
    ///
    /// let descendants = fragment.root_element().descendent_elements().map(|element| element.value().name()).collect::<Vec<_>>();
    /// assert_eq!(descendants, ["html", "span", "b", "a", "i"]);
    /// ```
    pub fn descendent_elements(&self) -> impl Iterator<Item = ElementRef<'a>> {
        self.descendants().filter_map(ElementRef::wrap)
    }
}

impl Debug for ElementRef<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        Debug::fmt(self.value(), f)
    }
}

impl<'a> Deref for ElementRef<'a> {
    type Target = NodeRef<'a, Node>;
    fn deref(&self) -> &NodeRef<'a, Node> {
        &self.node
    }
}

/// Iterator over descendent elements matching a selector.
pub struct Select<'a, 'b> {
    scope: ElementRef<'a>,
    inner: Traverse<'a, Node>,
    selector: &'b Selector,
    caches: SelectorCaches,
}

impl Debug for Select<'_, '_> {
    fn fmt(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt.debug_struct("Select")
            .field("scope", &self.scope)
            .field("inner", &self.inner)
            .field("selector", &self.selector)
            .field("caches", &"..")
            .finish()
    }
}

impl Clone for Select<'_, '_> {
    fn clone(&self) -> Self {
        Self {
            scope: self.scope,
            inner: self.inner.clone(),
            selector: self.selector,
            caches: Default::default(),
        }
    }
}

impl<'a> Iterator for Select<'a, '_> {
    type Item = ElementRef<'a>;

    fn next(&mut self) -> Option<ElementRef<'a>> {
        for edge in &mut self.inner {
            if let Edge::Open(node) = edge {
                if let Some(element) = ElementRef::wrap(node) {
                    if self.selector.matches_with_scope_and_cache(
                        &element,
                        Some(self.scope),
                        &mut self.caches,
                    ) {
                        return Some(element);
                    }
                }
            }
        }
        None
    }
}

impl FusedIterator for Select<'_, '_> {}

/// Iterator over descendent text nodes.
#[derive(Debug, Clone)]
pub struct Text<'a> {
    inner: Traverse<'a, Node>,
}

impl<'a> Iterator for Text<'a> {
    type Item = &'a str;

    fn next(&mut self) -> Option<&'a str> {
        for edge in &mut self.inner {
            if let Edge::Open(node) = edge {
                if let Node::Text(ref text) = node.value() {
                    return Some(&**text);
                }
            }
        }
        None
    }
}

impl FusedIterator for Text<'_> {}

mod element;
mod serializable;

#[cfg(test)]
mod tests {
    use crate::html::Html;
    use crate::selector::Selector;

    #[test]
    fn test_scope() {
        let html = r"
            <div>
                <b>1</b>
                <span>
                    <span><b>2</b></span>
                    <b>3</b>
                </span>
            </div>
        ";
        let fragment = Html::parse_fragment(html);
        let sel1 = Selector::parse("div > span").unwrap();
        let sel2 = Selector::parse(":scope > b").unwrap();

        let element1 = fragment.select(&sel1).next().unwrap();
        let element2 = element1.select(&sel2).next().unwrap();
        assert_eq!(element2.inner_html(), "3");
    }
}
