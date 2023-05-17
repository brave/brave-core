// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use lazy_static::lazy_static;
use regex::Regex;

#[cxx::bridge(namespace = brave_ads::rust)]
mod ffi {
    extern "Rust" {
        fn strip_non_alpha_characters(text: &str) -> String;
        fn strip_non_alpha_numeric_characters(text: &str) -> String;
        fn parse_and_sanitize_og_tag_attribute(html: &str) -> String;
        fn parse_html_og_tag_attribute(html: &str) -> String;
        fn collapse_whitespace(text: &str) -> String;
    }
}

fn strip_non_alpha_characters(text: &str) -> String {
    fn get_pattern() -> String {
        let escaped_characters = regex::escape("!\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~");
        format!(
            "[[:cntrl:]]|\\\\(t|n|v|f|r)|[\\t\\n\\v\\f\\r]|\\\\x[[:xdigit:]][[:xdigit:]]|[{}]|\\S*\\d+\\S*",
            escaped_characters
        )
    }

    lazy_static! {
        static ref CHAR_REGEX: Regex = Regex::new(&get_pattern()).unwrap();
    }
    let output = CHAR_REGEX.replace_all(text, " ");

    collapse_whitespace(&output)
}

fn strip_non_alpha_numeric_characters(text: &str) -> String {
    fn get_pattern() -> String {
        let escaped_characters = regex::escape("!\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~");
        format!(
            "[[:cntrl:]]|\\\\(t|n|v|f|r)|[\\t\\n\\v\\f\\r]|\\\\x[[:xdigit:]][[:xdigit:]]|[{}]",
            escaped_characters
        )
    }

    lazy_static! {
        static ref CHAR_REGEX: Regex = Regex::new(&get_pattern()).unwrap();
    }
    let output = CHAR_REGEX.replace_all(text, " ");

    collapse_whitespace(&output)
}

fn parse_and_sanitize_og_tag_attribute(html: &str) -> String {
    let tag_attribute = parse_html_og_tag_attribute(html);
    let sanitized_tag_attribute = strip_non_alpha_characters(&tag_attribute);
    sanitized_tag_attribute.to_lowercase()
}

fn parse_html_og_tag_attribute(html: &str) -> String {
    lazy_static! {
        static ref TAG_REGEX: Regex =
            Regex::new("(<[^>]*\\bproperty=[\"']og:title[\"'][^<]*>)").unwrap();
    }
    let tag_text = match TAG_REGEX.find(html) {
        Some(x) => x.as_str(),
        None => return String::new(),
    };

    lazy_static! {
        static ref ATTRIBUTE_REGEX: Regex =
            Regex::new("\\bcontent=(?P<delimiter>[\"'])(?P<attribute>.*>)").unwrap();
    }
    let caps = match ATTRIBUTE_REGEX.captures(tag_text) {
        Some(x) => x,
        None => return String::new(),
    };
    let delimiter: char = match caps.name("delimiter") {
        Some(x) => match x.as_str().chars().next() {
            Some(x) => x,
            None => return String::new(),
        },
        None => return String::new(),
    };
    let attribute = match caps.name("attribute") {
        Some(x) => x.as_str(),
        None => return String::new(),
    };

    let result: String = attribute.chars().take_while(|&c| c != delimiter).collect();
    match result.chars().last() {
        Some('>') | None => return String::new(),
        _ => result,
    }
}

fn collapse_whitespace(text: &str) -> String {
    let mut result = String::new();
    result.reserve(text.len());

    // Set flags to pretend we're already in a trimmed whitespace sequence, so we
    // will trim any leading whitespace.
    let mut in_whitespace = true;
    let mut already_trimmed = true;

    for c in text.chars() {
        if c.is_whitespace() {
            if !in_whitespace {
                // Reduce all whitespace sequences to a single space.
                in_whitespace = true;
                result.push(' ');
            }
            if !already_trimmed && (c == '\r' || c == '\n') {
                // Whitespace sequences containing CR or LF are eliminated entirely.
                already_trimmed = true;
                result.pop();
            }
        } else {
            // Non-whitespace characters are copied straight across.
            in_whitespace = false;
            already_trimmed = false;
            result.push(c);
        }
    }

    if (in_whitespace && !already_trimmed) {
        // Any trailing whitespace is eliminated.
        result.pop();
    }

    result
}
