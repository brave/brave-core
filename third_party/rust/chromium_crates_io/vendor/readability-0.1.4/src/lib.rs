#[macro_use]
extern crate html5ever;
extern crate regex;
extern crate url;
#[macro_use]
extern crate lazy_static;
#[cfg(feature = "reqwest")]
extern crate reqwest;

pub mod extractor;
pub mod scorer;
pub mod dom;
pub mod error;
