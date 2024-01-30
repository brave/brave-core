// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#[macro_use]
extern crate html5ever;
extern crate kuchikiki;
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
