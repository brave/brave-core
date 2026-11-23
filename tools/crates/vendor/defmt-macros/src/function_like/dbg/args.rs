use syn::{
    parse::{Parse, ParseStream},
    punctuated::Punctuated,
    Expr, Token,
};

pub(crate) struct Args {
    pub(crate) exprs: Punctuated<Expr, Token![,]>,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> Result<Self, syn::Error> {
        Ok(Self {
            exprs: Punctuated::parse_terminated(input)?,
        })
    }
}
