// Copyright (c) 2016 The Rouille developers
// Licensed under the Apache License, Version 2.0
// <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT
// license <LICENSE-MIT or http://opensource.org/licenses/MIT>,
// at your option. All files in the project carrying such
// notice may not be copied, modified, or distributed except
// according to those terms.

use std::f32;
use std::str::FromStr;
use std::str::Split;

/// Returns the preferred value amongst a priority header.
///
/// This function takes the value of a priority header and a list of elements that can be handled
/// by the server, and returns the index within that list of the element with the highest priority.
///
/// If multiple elements in `handled` match and have the same priority, the first one is returned.
///
/// # Example
///
/// ```
/// use rouille::input::priority_header_preferred;
///
/// let header = "text/plain; q=1.2, image/png; q=2.0";
/// let handled = ["image/gif", "image/png", "text/plain"];
/// assert_eq!(priority_header_preferred(header, handled.iter().cloned()), Some(1));
/// ```
pub fn priority_header_preferred<'a, I>(input: &'a str, elements: I) -> Option<usize>
where
    I: Iterator<Item = &'a str>,
{
    let mut result = (None, f32::NEG_INFINITY);

    for (index, req_elem) in elements.enumerate() {
        for (header_elem, prio) in parse_priority_header(input) {
            if prio <= result.1 {
                continue;
            }

            if req_elem == header_elem {
                result = (Some(index), prio);
                continue;
            }

            let (req_elem_left, req_elem_right) = {
                let mut parts = req_elem.split('/');
                let left = parts.next();
                let right = parts.next();
                (left, right)
            };

            let (header_elem_left, header_elem_right) = {
                let mut parts = header_elem.split('/');
                let left = parts.next();
                let right = parts.next();
                (left, right)
            };

            if (req_elem_left == Some("*") || header_elem_left == Some("*"))
                && (req_elem_right == header_elem_right
                    || req_elem_right == Some("*")
                    || header_elem_right == Some("*"))
            {
                result = (Some(index), prio);
                continue;
            }

            if (req_elem_right == Some("*") || header_elem_right == Some("*"))
                && (req_elem_left == header_elem_left
                    || req_elem_left == Some("*")
                    || header_elem_left == Some("*"))
            {
                result = (Some(index), prio);
                continue;
            }
        }
    }

    result.0
}

/// Parses the value of a header that has values with priorities. Suitable for
/// `Accept-*`, `TE`, etc.
///
/// # Example
///
/// ```
/// use rouille::input::parse_priority_header;
///
/// let mut iter = parse_priority_header("text/plain, image/png; q=1.5");
///
/// assert_eq!(iter.next().unwrap(), ("text/plain", 1.0));
/// assert_eq!(iter.next().unwrap(), ("image/png", 1.5));
/// assert_eq!(iter.next(), None);
/// ```
#[inline]
pub fn parse_priority_header(input: &str) -> PriorityHeaderIter {
    PriorityHeaderIter {
        iter: input.split(','),
    }
}

/// Iterator to the elements of a priority header.
///
/// Created with [`parse_priority_header`](fn.parse_priority_header.html).
pub struct PriorityHeaderIter<'a> {
    iter: Split<'a, char>,
}

impl<'a> Iterator for PriorityHeaderIter<'a> {
    type Item = (&'a str, f32);

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            let elem = match self.iter.next() {
                Some(n) => n,
                None => return None,
            };

            let mut params = elem.split(';');

            let t = match params.next() {
                Some(t) => t.trim(),
                None => continue,
            };

            let mut value = 1.0f32;

            for p in params {
                let trimmed_p = p.trim_start();
                if let Some(stripped) = trimmed_p.strip_prefix("q=") {
                    if let Ok(val) = FromStr::from_str(stripped.trim()) {
                        value = val;
                        break;
                    }
                }
            }

            return Some((t, value));
        }
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let (_, len) = self.iter.size_hint();
        (0, len)
    }
}

#[cfg(test)]
mod tests {
    use super::parse_priority_header;
    use super::priority_header_preferred;

    #[test]
    fn parse_basic() {
        let mut iter = parse_priority_header("text/plain; q=1.5, */*");
        assert_eq!(iter.next().unwrap(), ("text/plain", 1.5));
        assert_eq!(iter.next().unwrap(), ("*/*", 1.0));
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn parse_white_spaces() {
        let mut iter = parse_priority_header("   text/plain   ;  q=   1.5  ,    */*   ");
        assert_eq!(iter.next().unwrap(), ("text/plain", 1.5));
        assert_eq!(iter.next().unwrap(), ("*/*", 1.0));
        assert_eq!(iter.next(), None);
    }

    #[test]
    fn preferred_basic() {
        let header = "text/plain; q=1.2, image/png; q=2.0";
        let handled = ["image/gif", "image/png", "text/plain"];
        assert_eq!(
            priority_header_preferred(header, handled.iter().cloned()),
            Some(1)
        );
    }

    #[test]
    fn preferred_multimatch_first() {
        let header = "text/plain";
        let handled = ["text/plain", "text/plain"];
        assert_eq!(
            priority_header_preferred(header, handled.iter().cloned()),
            Some(0)
        );
    }

    #[test]
    fn preferred_wildcard_header() {
        let header = "text/plain; q=1.2, */*";
        let handled = ["image/gif"];
        assert_eq!(
            priority_header_preferred(header, handled.iter().cloned()),
            Some(0)
        );
    }

    #[test]
    fn preferred_wildcard_header_left() {
        let header = "text/*; q=2.0, */*";
        let handled = ["image/gif", "text/html"];
        assert_eq!(
            priority_header_preferred(header, handled.iter().cloned()),
            Some(1)
        );
    }

    #[test]
    fn preferred_empty() {
        let header = "*/*";
        let handled = [];
        assert_eq!(
            priority_header_preferred(header, handled.iter().cloned()),
            None
        );
    }
}
