use syn::{
    parse::{self, Parse, ParseStream},
    Expr, Token,
};

use crate::function_like::log;

pub(crate) struct Args {
    pub(crate) formatter: Expr,
    _comma: Token![,],
    pub(crate) log_args: log::Args,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> parse::Result<Self> {
        Ok(Self {
            formatter: input.parse()?,
            _comma: input.parse()?,
            log_args: input.parse()?,
        })
    }
}
