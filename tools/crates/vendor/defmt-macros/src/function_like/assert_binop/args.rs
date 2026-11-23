use syn::{
    parse::{self, Parse, ParseStream},
    Expr, Token,
};

use crate::function_like::log;

pub(crate) struct Args {
    pub(crate) left: Expr,
    pub(crate) right: Expr,
    pub(crate) log_args: Option<log::Args>,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> parse::Result<Self> {
        let left = input.parse()?;
        let _comma: Token![,] = input.parse()?;
        let right = input.parse()?;

        if input.is_empty() {
            // assert_eq!(a, b)
            return Ok(Args {
                left,
                right,
                log_args: None,
            });
        }

        let _comma: Token![,] = input.parse()?;

        if input.is_empty() {
            // assert_eq!(a, b,)
            Ok(Args {
                left,
                right,
                log_args: None,
            })
        } else {
            // assert_eq!(a, b, "c", d)
            Ok(Args {
                left,
                right,
                log_args: Some(input.parse()?),
            })
        }
    }
}
