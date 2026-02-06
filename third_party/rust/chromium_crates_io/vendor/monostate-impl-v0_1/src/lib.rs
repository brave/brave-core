#![cfg_attr(not(check_cfg), allow(unexpected_cfgs))]
#![allow(
    clippy::cast_lossless,
    clippy::manual_range_contains,
    clippy::match_same_arms,
    clippy::needless_pass_by_value,
    clippy::uninlined_format_args,
    clippy::unnecessary_wraps
)]
#![cfg_attr(all(test, exhaustive), feature(non_exhaustive_omitted_patterns_lint))]

use proc_macro::TokenStream;
use proc_macro2::{Ident, Literal, Span, TokenStream as TokenStream2};
use quote::{quote, ToTokens};
use std::mem;
use syn::{parse_macro_input, Error, Lit, LitInt, Result};

// Branching factor of the MustBeStr tuple type parameter.
const K: usize = 6;

#[allow(non_snake_case)]
#[proc_macro]
pub fn MustBe(input: TokenStream) -> TokenStream {
    let lit = parse_macro_input!(input as Lit);

    #[cfg_attr(all(test, exhaustive), deny(non_exhaustive_omitted_patterns))]
    let expanded = match lit {
        Lit::Str(lit) => must_be_str(lit.value()),
        Lit::Byte(lit) => must_be_byte(lit.value()),
        Lit::Char(lit) => must_be_char(lit.value()),
        Lit::Int(lit) => must_be_int(lit),
        Lit::Bool(lit) => must_be_bool(lit.value),
        Lit::ByteStr(_) | Lit::CStr(_) | Lit::Float(_) | Lit::Verbatim(_) => unsupported(lit),
        _ => unsupported(lit),
    };

    expanded.unwrap_or_else(Error::into_compile_error).into()
}

// We encode the chars at two consecutive levels of a K-ary tree.
//
// Suppose K=3, then strings "", "a", "ab", "abc", … would be encoded to:
//     ()
//     (a)
//     (a, b)
//     (a, b, c)
//     (a, b, (c, d))
//     (a, b, (c, d, e))
//     (a, (b, c), (d, e, f))
//     (a, (b, c, d), (e, f, g))
//     ((a, b), (c, d, e), (f, g, h))
//     ((a, b, c), (d, e, f), (g, h, i))
//     ((a, b, c), (d, e, f), (g, h, (i, j)))
//     ((a, b, c), (d, e, f), (g, h, (i, j, k)))
//     ((a, b, c), (d, e, f), (g, (h, i), (j, k l)))
//
// That last one in tree form is:
//           ╷
//      ┌────┴┬──────┐
//     ┌┴┬─┐ ┌┴┬─┐ ┌─┴┬───┐
//     a b c d e f g ┌┴┐ ┌┴┬─┐
//                   h i j k l

enum StrNode {
    Char(char),
    Tuple(Vec<StrNode>),
}

impl ToTokens for StrNode {
    fn to_tokens(&self, tokens: &mut TokenStream2) {
        tokens.extend(match self {
            StrNode::Char(ch) => {
                if let 'A'..='Z' | 'a'..='z' = ch {
                    let mut buf = [0];
                    let name = ch.encode_utf8(&mut buf);
                    let ident = Ident::new(name, Span::call_site());
                    quote!(::monostate::alphabet::#ident)
                } else {
                    match ch.len_utf8() {
                        1 => quote!(::monostate::alphabet::char<#ch>),
                        2 => quote!(::monostate::alphabet::two::char<#ch>),
                        3 => quote!(::monostate::alphabet::three::char<#ch>),
                        4 => quote!(::monostate::alphabet::four::char<#ch>),
                        _ => unreachable!(),
                    }
                }
            }
            StrNode::Tuple(vec) => {
                let len = vec.len();
                assert!(len >= 2 && len <= K, "len={}", len);
                quote!((#(#vec),*))
            }
        });
    }
}

fn must_be_str(value: String) -> Result<TokenStream2> {
    if value.is_empty() {
        return Ok(quote! {
            ::monostate::MustBeStr::<(::monostate::alphabet::len<0>, ())>
        });
    }
    let mut nodes = Vec::new();
    for ch in value.chars() {
        nodes.push(StrNode::Char(ch));
    }
    // Find largest power of K smaller than len.
    let mut pow = 1;
    while pow * K < nodes.len() {
        pow *= K;
    }
    while nodes.len() > 1 {
        // Number of nodes in excess of the smaller of the two tree levels.
        let overage = nodes.len() - pow;
        // Every group of K-1 nodes which are beyond the smaller tree level can
        // be combined with 1 node from the smaller tree level to form a
        // K-tuple node. The number of tuples that need to be formed is
        // ceil[overage / (K-1)].
        let num_tuple_nodes = (overage + K - 2) / (K - 1);
        // Number of nodes left needing to be inserted into a tuple.
        let mut remainder = num_tuple_nodes + overage;
        // Index of next node to be inserted into a tuple.
        let mut read = nodes.len() - remainder;
        // Index of the tuple currently being inserted into.
        let mut write = read;
        // True if we haven't yet made a Vec to hold the current tuple.
        let mut make_tuple = true;
        while let Some(node) = nodes.get_mut(read) {
            let next = mem::replace(node, StrNode::Char('\0'));
            if make_tuple {
                nodes[write] = StrNode::Tuple(Vec::with_capacity(K));
            }
            if let StrNode::Tuple(vec) = &mut nodes[write] {
                vec.push(next);
            } else {
                unreachable!();
            }
            remainder -= 1;
            make_tuple = remainder % K == 0;
            write += make_tuple as usize;
            read += 1;
        }
        nodes.truncate(pow);
        pow /= K;
    }
    let len = Literal::usize_unsuffixed(value.len());
    let encoded = &nodes[0];
    Ok(quote! {
        ::monostate::MustBeStr::<(::monostate::alphabet::len<#len>, #encoded)>
    })
}

fn must_be_byte(value: u8) -> Result<TokenStream2> {
    Ok(quote!(::monostate::MustBeU8::<#value>))
}

fn must_be_char(value: char) -> Result<TokenStream2> {
    Ok(quote!(::monostate::MustBeChar::<#value>))
}

fn must_be_int(lit: LitInt) -> Result<TokenStream2> {
    let token = lit.token();
    match lit.suffix() {
        "u8" => Ok(quote!(::monostate::MustBeU8::<#token>)),
        "u16" => Ok(quote!(::monostate::MustBeU16::<#token>)),
        "u32" => Ok(quote!(::monostate::MustBeU32::<#token>)),
        "u64" => Ok(quote!(::monostate::MustBeU64::<#token>)),
        "u128" => Ok(quote!(::monostate::MustBeU128::<#token>)),
        "i8" => Ok(quote!(::monostate::MustBeI8::<#token>)),
        "i16" => Ok(quote!(::monostate::MustBeI16::<#token>)),
        "i32" => Ok(quote!(::monostate::MustBeI32::<#token>)),
        "i64" => Ok(quote!(::monostate::MustBeI64::<#token>)),
        "i128" => Ok(quote!(::monostate::MustBeI128::<#token>)),
        "" => {
            if lit.base10_digits().starts_with('-') {
                Ok(quote!(::monostate::MustBeNegInt::<#token>))
            } else {
                Ok(quote!(::monostate::MustBePosInt::<#token>))
            }
        }
        suffix @ ("usize" | "isize") => {
            let msg = format!(
                "serde data model only uses consistently sized integer types, not {}",
                suffix,
            );
            Err(Error::new(lit.span(), msg))
        }
        suffix => {
            let msg = format!("unsupported integers suffix `{}`", suffix);
            Err(Error::new(lit.span(), msg))
        }
    }
}

fn must_be_bool(value: bool) -> Result<TokenStream2> {
    Ok(quote!(::monostate::MustBeBool::<#value>))
}

fn unsupported(lit: Lit) -> Result<TokenStream2> {
    Err(Error::new(lit.span(), "unsupported monostate literal kind"))
}
