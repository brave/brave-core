use proc_macro::TokenStream;
use syn::{parse_macro_input, parse_quote, LitStr};

use crate::construct;

pub(crate) fn expand(args: TokenStream) -> TokenStream {
    let literal = parse_macro_input!(args as LitStr);
    construct::interned_string(&literal.value(), "str", false, None, &parse_quote!(defmt)).into()
}
