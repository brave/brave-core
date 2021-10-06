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

pub use self::speedreader::{
    OutputSink, RewriterType, SpeedReader, SpeedReaderError, SpeedReaderProcessor,
};
