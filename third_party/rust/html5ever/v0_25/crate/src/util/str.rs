// Copyright 2014-2017 The html5ever Project Developers. See the
// COPYRIGHT file at the top-level directory of this distribution.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use mac::{matches, _tt_as_expr_hack};
use std::fmt;

pub fn to_escaped_string<T: fmt::Debug>(x: &T) -> String {
    // FIXME: don't allocate twice
    let string = format!("{:?}", x);
    string.chars().flat_map(|c| c.escape_default()).collect()
}

/// If `c` is an ASCII letter, return the corresponding lowercase
/// letter, otherwise None.
pub fn lower_ascii_letter(c: char) -> Option<char> {
    match c {
        'a'..='z' => Some(c),
        'A'..='Z' => Some((c as u8 - b'A' + b'a') as char),
        _ => None,
    }
}

/// Is the character an ASCII alphanumeric character?
pub fn is_ascii_alnum(c: char) -> bool {
    matches!(c, '0'..='9' | 'a'..='z' | 'A'..='Z')
}

/// ASCII whitespace characters, as defined by
/// tree construction modes that treat them specially.
pub fn is_ascii_whitespace(c: char) -> bool {
    matches!(c, '\t' | '\r' | '\n' | '\x0C' | ' ')
}

#[cfg(test)]
#[allow(non_snake_case)]
mod test {
    use super::{is_ascii_alnum, lower_ascii_letter};
    use mac::test_eq;

    test_eq!(lower_letter_a_is_a, lower_ascii_letter('a'), Some('a'));
    test_eq!(lower_letter_A_is_a, lower_ascii_letter('A'), Some('a'));
    test_eq!(lower_letter_symbol_is_None, lower_ascii_letter('!'), None);
    test_eq!(
        lower_letter_nonascii_is_None,
        lower_ascii_letter('\u{a66e}'),
        None
    );

    test_eq!(is_alnum_a, is_ascii_alnum('a'), true);
    test_eq!(is_alnum_A, is_ascii_alnum('A'), true);
    test_eq!(is_alnum_1, is_ascii_alnum('1'), true);
    test_eq!(is_not_alnum_symbol, is_ascii_alnum('!'), false);
    test_eq!(is_not_alnum_nonascii, is_ascii_alnum('\u{a66e}'), false);
}
