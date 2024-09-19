//! Internal library for data-encoding-macro
//!
//! Do **not** use this library. Use [data-encoding-macro] instead.
//!
//! This library is for internal use by data-encoding-macro because procedural
//! macros require a separate crate.
//!
//! [data-encoding-macro]: https://crates.io/crates/data-encoding-macro

#![warn(unused_results)]

use proc_macro::token_stream::IntoIter;
use proc_macro::{TokenStream, TokenTree};
use std::collections::HashMap;

use data_encoding::{BitOrder, Encoding, Specification, Translate, Wrap};

fn parse_op(tokens: &mut IntoIter, op: char, key: &str) {
    match tokens.next() {
        Some(TokenTree::Punct(ref x)) if x.as_char() == op => (),
        _ => panic!("expected {:?} after {}", op, key),
    }
}

fn parse_map(mut tokens: IntoIter) -> HashMap<String, TokenTree> {
    let mut map = HashMap::new();
    while let Some(key) = tokens.next() {
        let key = match key {
            TokenTree::Ident(ident) => format!("{}", ident),
            _ => panic!("expected key got {}", key),
        };
        parse_op(&mut tokens, ':', &key);
        let value = match tokens.next() {
            None => panic!("expected value for {}", key),
            Some(value) => value,
        };
        parse_op(&mut tokens, ',', &key);
        let _ = map.insert(key, value);
    }
    map
}

fn get_string(map: &mut HashMap<String, TokenTree>, key: &str) -> String {
    let node = match map.remove(key) {
        None => return String::new(),
        Some(node) => node,
    };
    match syn::parse::<syn::LitStr>(node.into()) {
        Ok(result) => result.value(),
        _ => panic!("expected string for {}", key),
    }
}

fn get_usize(map: &mut HashMap<String, TokenTree>, key: &str) -> usize {
    let node = match map.remove(key) {
        None => return 0,
        Some(node) => node,
    };
    let literal = match node {
        TokenTree::Literal(literal) => literal,
        _ => panic!("expected literal for {}", key),
    };
    match literal.to_string().parse() {
        Ok(result) => result,
        Err(error) => panic!("expected usize for {}: {}", key, error),
    }
}

fn get_padding(map: &mut HashMap<String, TokenTree>) -> Option<char> {
    let node = match map.remove("padding") {
        None => return None,
        Some(node) => node,
    };
    if let Ok(result) = syn::parse::<syn::LitChar>(node.clone().into()) {
        return Some(result.value());
    }
    match syn::parse::<syn::Ident>(node.into()) {
        Ok(ref result) if result == "None" => None,
        _ => panic!("expected None or char for padding"),
    }
}

fn get_bool(map: &mut HashMap<String, TokenTree>, key: &str) -> Option<bool> {
    let node = match map.remove(key) {
        None => return None,
        Some(node) => node,
    };
    match syn::parse::<syn::LitBool>(node.into()) {
        Ok(result) => Some(result.value),
        _ => panic!("expected bool for padding"),
    }
}

fn get_bit_order(map: &mut HashMap<String, TokenTree>) -> BitOrder {
    let node = match map.remove("bit_order") {
        None => return BitOrder::MostSignificantFirst,
        Some(node) => node,
    };
    let msb = "MostSignificantFirst";
    let lsb = "LeastSignificantFirst";
    match node {
        TokenTree::Ident(ref ident) if format!("{}", ident) == msb => {
            BitOrder::MostSignificantFirst
        }
        TokenTree::Ident(ref ident) if format!("{}", ident) == lsb => {
            BitOrder::LeastSignificantFirst
        }
        _ => panic!("expected {} or {} for bit_order", msb, lsb),
    }
}

fn check_present<T>(hash_map: &HashMap<String, T>, key: &str) {
    if !hash_map.contains_key(key) {
        panic!("{} is required", key);
    }
}

fn get_encoding(mut hash_map: &mut HashMap<String, TokenTree>) -> Encoding {
    check_present(&hash_map, "symbols");
    let spec = Specification {
        symbols: get_string(&mut hash_map, "symbols"),
        bit_order: get_bit_order(&mut hash_map),
        check_trailing_bits: get_bool(&mut hash_map, "check_trailing_bits").unwrap_or(true),
        padding: get_padding(&mut hash_map),
        ignore: get_string(&mut hash_map, "ignore"),
        wrap: Wrap {
            width: get_usize(&mut hash_map, "wrap_width"),
            separator: get_string(&mut hash_map, "wrap_separator"),
        },
        translate: Translate {
            from: get_string(&mut hash_map, "translate_from"),
            to: get_string(&mut hash_map, "translate_to"),
        },
    };
    spec.encoding().unwrap()
}

fn check_empty<T>(hash_map: HashMap<String, T>) {
    if !hash_map.is_empty() {
        panic!("Unexpected keys {:?}", hash_map.keys());
    }
}

#[proc_macro]
#[doc(hidden)]
pub fn internal_new_encoding(input: TokenStream) -> TokenStream {
    let mut hash_map = parse_map(input.into_iter());
    let encoding = get_encoding(&mut hash_map);
    check_empty(hash_map);
    format!("{:?}", encoding.internal_implementation()).parse().unwrap()
}

#[proc_macro]
#[doc(hidden)]
pub fn internal_decode_array(input: TokenStream) -> TokenStream {
    let mut hash_map = parse_map(input.into_iter());
    let encoding = get_encoding(&mut hash_map);
    check_present(&hash_map, "name");
    let name = get_string(&mut hash_map, "name");
    check_present(&hash_map, "input");
    let input = get_string(&mut hash_map, "input");
    check_empty(hash_map);
    let output = encoding.decode(input.as_bytes()).unwrap();
    format!("{}: [u8; {}] = {:?};", name, output.len(), output).parse().unwrap()
}

#[proc_macro]
#[doc(hidden)]
pub fn internal_decode_slice(input: TokenStream) -> TokenStream {
    let mut hash_map = parse_map(input.into_iter());
    let encoding = get_encoding(&mut hash_map);
    check_present(&hash_map, "input");
    let input = get_string(&mut hash_map, "input");
    check_empty(hash_map);
    format!("{:?}", encoding.decode(input.as_bytes()).unwrap()).parse().unwrap()
}
