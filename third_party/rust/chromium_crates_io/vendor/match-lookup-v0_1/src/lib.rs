use proc_macro::{self, TokenStream};
use quote::ToTokens;
use syn::{
    parse::Parse, parse_macro_input, parse_quote, Arm, Expr,
    ExprMatch, LitStr, Token, LitByteStr,
};

struct CharMatch {
    expr: Box<Expr>,
    alphabet: LitStr,
}

struct AsciiMatch {
    expr: Box<Expr>,
    alphabet: LitByteStr,
}

impl Parse for CharMatch {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        let expr: Box<Expr> = input.parse()?;
        input.parse::<Token![,]>()?;
        let alphabet: LitStr = input.parse()?;

        Ok(CharMatch {
            expr,
            alphabet,
        })
    }
}

impl Parse for AsciiMatch {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        let expr: Box<Expr> = input.parse()?;
        input.parse::<Token![,]>()?;
        let alphabet: LitByteStr = input.parse()?;

        Ok(AsciiMatch {
            expr,
            alphabet,
        })
    }
}

#[proc_macro]
pub fn gen_char_match(input: TokenStream) -> TokenStream {
    let CharMatch {
        expr,
        alphabet,
    } = parse_macro_input!(input as CharMatch);

    let cases = alphabet.value();

    let mut arms: Vec<Arm> = cases
        .chars()
        .enumerate()
        .map(|(i, ch)| -> Arm { parse_quote!(#ch => Some(#i),) })
        .collect();

    arms.push(parse_quote!(_ => None,));

let match_exp = ExprMatch {
        attrs: Default::default(),
        match_token: Default::default(),
        brace_token: Default::default(),
        expr,
        arms,
    };

    match_exp.into_token_stream().into()
}

#[proc_macro]
pub fn gen_ascii_match(input: TokenStream) -> TokenStream {
    let AsciiMatch {
        expr,
        alphabet,
    } = parse_macro_input!(input as AsciiMatch);

    let cases = alphabet.value();

    if !cases.as_slice().is_ascii() {
        panic!("must use an ascii string")
    }

    let mut arms: Vec<Arm> = cases
        .into_iter()
        .enumerate()
        .map(|(i, ch)| -> Arm { parse_quote!(#ch => Some(#i),) })
        .collect();

    arms.push(parse_quote!(_ => None,));

let match_exp = ExprMatch {
        attrs: Default::default(),
        match_token: Default::default(),
        brace_token: Default::default(),
        expr,
        arms,
    };

    match_exp.into_token_stream().into()
}
