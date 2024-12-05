use super::compiler::AttrExprOperands;
use crate::base::Bytes;
use crate::html::Namespace;
use crate::parser::{AttributeBuffer, AttributeOutline};
use encoding_rs::UTF_8;
use lazy_static::lazy_static;
use lazycell::LazyCell;
use memchr::{memchr, memchr2};
use selectors::attr::CaseSensitivity;

lazy_static! {
    static ref ID_ATTR: Bytes<'static> = Bytes::from_str("id", UTF_8);
    static ref CLASS_ATTR: Bytes<'static> = Bytes::from_str("class", UTF_8);
}

#[inline]
const fn is_attr_whitespace(b: u8) -> bool {
    b == b' ' || b == b'\n' || b == b'\r' || b == b'\t' || b == b'\x0c'
}

type MemoizedAttrValue<'i> = LazyCell<Option<Bytes<'i>>>;

pub(crate) struct AttributeMatcher<'i> {
    input: &'i Bytes<'i>,
    attributes: &'i AttributeBuffer,
    id: MemoizedAttrValue<'i>,
    class: MemoizedAttrValue<'i>,
    is_html_element: bool,
}

impl<'i> AttributeMatcher<'i> {
    #[inline]
    #[must_use]
    pub fn new(input: &'i Bytes<'i>, attributes: &'i AttributeBuffer, ns: Namespace) -> Self {
        AttributeMatcher {
            input,
            attributes,
            id: LazyCell::default(),
            class: LazyCell::default(),
            is_html_element: ns == Namespace::Html,
        }
    }

    #[inline]
    fn find(&self, lowercased_name: &Bytes<'_>) -> Option<AttributeOutline> {
        self.attributes
            .iter()
            .find(|a| {
                if lowercased_name.len() != a.name.end - a.name.start {
                    return false;
                }

                let attr_name = self.input.slice(a.name);

                for i in 0..attr_name.len() {
                    if attr_name[i].to_ascii_lowercase() != lowercased_name[i] {
                        return false;
                    }
                }

                true
            })
            .copied()
    }

    #[inline]
    fn get_value(&self, lowercased_name: &Bytes<'_>) -> Option<Bytes<'i>> {
        self.find(lowercased_name)
            .map(|a| self.input.slice(a.value))
    }

    #[inline]
    #[must_use]
    pub fn has_attribute(&self, lowercased_name: &Bytes<'_>) -> bool {
        self.find(lowercased_name).is_some()
    }

    #[inline]
    #[must_use]
    pub fn has_id(&self, id: &Bytes<'_>) -> bool {
        match self.id.borrow_with(|| self.get_value(&ID_ATTR)) {
            Some(actual_id) => actual_id == id,
            None => false,
        }
    }

    #[inline]
    #[must_use]
    pub fn has_class(&self, class_name: &Bytes<'_>) -> bool {
        match self.class.borrow_with(|| self.get_value(&CLASS_ATTR)) {
            Some(class) => class
                .split(|&b| is_attr_whitespace(b))
                .any(|actual_class_name| actual_class_name == &**class_name),
            None => false,
        }
    }

    #[inline]
    fn value_matches(&self, name: &Bytes<'_>, matcher: impl Fn(Bytes<'_>) -> bool) -> bool {
        self.get_value(name).is_some_and(matcher)
    }

    #[inline]
    pub fn attr_eq(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            operand
                .case_sensitivity
                .to_unconditional(self.is_html_element)
                .eq(&actual_value, &operand.value)
        })
    }

    #[inline]
    pub fn matches_splitted_by_whitespace(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            let case_sensitivity = operand
                .case_sensitivity
                .to_unconditional(self.is_html_element);

            actual_value
                .split(|&b| is_attr_whitespace(b))
                .any(|part| case_sensitivity.eq(part, &operand.value))
        })
    }

    #[inline]
    pub fn has_attr_with_prefix(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            let case_sensitivity = operand
                .case_sensitivity
                .to_unconditional(self.is_html_element);

            let prefix_len = operand.value.len();

            actual_value.len() >= prefix_len
                && case_sensitivity.eq(&actual_value[..prefix_len], &operand.value)
        })
    }

    #[inline]
    pub fn has_dash_matching_attr(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            let case_sensitivity = operand
                .case_sensitivity
                .to_unconditional(self.is_html_element);

            if case_sensitivity.eq(&actual_value, &operand.value) {
                return true;
            }

            let prefix_len = operand.value.len();

            actual_value.get(prefix_len) == Some(&b'-')
                && case_sensitivity.eq(&actual_value[..prefix_len], &operand.value)
        })
    }

    #[inline]
    pub fn has_attr_with_suffix(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            let case_sensitivity = operand
                .case_sensitivity
                .to_unconditional(self.is_html_element);

            let suffix_len = operand.value.len();
            let value_len = actual_value.len();

            value_len >= suffix_len
                && case_sensitivity.eq(&actual_value[value_len - suffix_len..], &operand.value)
        })
    }

    #[inline]
    pub fn has_attr_with_substring(&self, operand: &AttrExprOperands) -> bool {
        self.value_matches(&operand.name, |actual_value| {
            let case_sensitivity = operand
                .case_sensitivity
                .to_unconditional(self.is_html_element);

            let Some((&first_byte, rest)) = operand.value.split_first() else {
                return false;
            };

            let first_byte_searcher: Box<dyn Fn(_) -> _> = match case_sensitivity {
                CaseSensitivity::CaseSensitive => Box::new(|h| memchr(first_byte, h)),
                CaseSensitivity::AsciiCaseInsensitive => {
                    let lo = first_byte.to_ascii_lowercase();
                    let up = first_byte.to_ascii_uppercase();

                    Box::new(move |h| memchr2(lo, up, h))
                }
            };

            let mut haystack = &*actual_value;

            loop {
                match first_byte_searcher(haystack) {
                    Some(pos) => {
                        haystack = &haystack[pos + 1..];

                        if haystack.len() < rest.len() {
                            return false;
                        }

                        if case_sensitivity.eq(&haystack[..rest.len()], rest) {
                            return true;
                        }
                    }
                    None => return false,
                }
            }
        })
    }
}
