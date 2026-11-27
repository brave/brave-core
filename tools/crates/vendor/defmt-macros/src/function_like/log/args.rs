use syn::{
    parse::{self, Parse, ParseStream},
    punctuated::Punctuated,
    Expr, LitStr, Token,
};

pub(crate) struct Args {
    pub(crate) format_string: LitStr,
    pub(crate) formatting_args: Option<Punctuated<Expr, Token![,]>>,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> parse::Result<Self> {
        Ok(Self {
            format_string: input.parse()?,
            formatting_args: if input.is_empty() {
                None
            } else {
                let _comma: Token![,] = input.parse()?;
                Some(Punctuated::parse_terminated(input)?)
            },
        })
    }
}
