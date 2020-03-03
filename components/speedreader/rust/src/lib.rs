#![allow(dead_code)]
#![forbid(unsafe_code)]
extern crate adblock;
extern crate html5ever;
extern crate lol_html;
extern crate url;

#[cfg(test)]
#[macro_use]
extern crate matches;

extern crate rmp_serde as rmps; // binary serialization/deserialization

pub mod classifier;
mod rewriter_config_builder;
pub mod speedreader;
mod speedreader_heuristics;
mod speedreader_streaming;

pub mod whitelist;

pub use self::speedreader::{
    AttributeRewrite, OutputSink, RewriteRules, RewriterType, SpeedReader, SpeedReaderConfig,
    SpeedReaderError, SpeedReaderProcessor,
};
