#![allow(dead_code)]
#![forbid(unsafe_code)]
extern crate html5ever;
extern crate lol_html;
extern crate url;

#[cfg(test)]
#[macro_use]
extern crate matches;

pub mod classifier;
pub mod speedreader;
mod speedreader_heuristics;
mod speedreader_readability;

pub mod whitelist;

pub use self::speedreader::{
    AttributeRewrite, OutputSink, RewriteRules, RewriterType, SpeedReader, SpeedReaderConfig,
    SpeedReaderError, SpeedReaderProcessor,
};
