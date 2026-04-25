/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#![allow(dead_code)]
#![forbid(unsafe_code)]
extern crate html5ever;
extern crate lol_html;
extern crate url;

#[cfg(test)]
#[macro_use]
extern crate matches;

pub mod speedreader;
mod speedreader_readability;

pub use self::speedreader::{OutputSink, SpeedReader, SpeedReaderError, SpeedReaderProcessor};
