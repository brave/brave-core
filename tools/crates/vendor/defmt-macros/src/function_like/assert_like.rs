use syn::{
    parse::{self, Parse, ParseStream},
    Expr, Token,
};

use super::log;

pub(crate) mod assert;
pub(crate) mod unwrap;

struct Args {
    condition: Expr,
    log_args: Option<log::Args>,
}

impl Parse for Args {
    fn parse(input: ParseStream) -> parse::Result<Self> {
        let condition = input.parse()?;
        if input.is_empty() {
            // assert!(a)
            return Ok(Args {
                log_args: None,
                condition,
            });
        }

        let _comma: Token![,] = input.parse()?;

        if input.is_empty() {
            // assert!(a,)
            Ok(Args {
                log_args: None,
                condition,
            })
        } else {
            // assert!(a, "b", c)
            Ok(Args {
                log_args: Some(input.parse()?),
                condition,
            })
        }
    }
}
