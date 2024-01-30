#[macro_use]
extern crate html5ever;
extern crate kuchiki;
extern crate regex;
extern crate url;
#[macro_use]
extern crate lazy_static;

pub mod dom;
pub mod error;
pub mod extractor;
mod nlp;
pub mod scorer;
pub mod statistics;
mod util;
