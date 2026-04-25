// Copyright 2016 The rust-url developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::{error::Error as StdError, fmt};

#[derive(Default, Debug)]
pub struct Errors {}

impl From<Errors> for Result<(), Errors> {
    fn from(e: Errors) -> Result<(), Errors> {
        Err(e)
    }
}

impl StdError for Errors {}

impl fmt::Display for Errors {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        fmt::Debug::fmt(self, f)
    }
}

#[cxx::bridge(namespace = "idna")]
mod ffi {
    struct IdnaResult {
      domain: String,
      valid: bool
    }

    unsafe extern "C++" {
        include!("brave/third_party/rust/idna/v1/crate/idna.h");
        #[allow(dead_code)]
        fn InitializeICUForTesting();
        fn DomainToASCII(domain: &str) -> IdnaResult;
    }
}

pub fn domain_to_ascii(domain: &str) -> Result<String, Errors> {
    let res = ffi::DomainToASCII(domain);
    match res.valid {
        false => Err(Errors::default()),
        true => Ok(res.domain)
    }
}

// Tests from original idna crate
// To build tests run
// `npm run build -- --target=brave/third_party/rust/idna/v0_2:lib_idna_v0_2_unittests`
// and then run `out_dir/lib_idna_v0_2_unittests`
#[test]
fn test_domain_to_ascii() {
    ffi::InitializeICUForTesting();
    assert_eq!(domain_to_ascii("abc").unwrap(), "abc");
    assert_eq!(domain_to_ascii("123").unwrap(), "123");
    assert_eq!(domain_to_ascii("אבּג").unwrap(), "xn--kdb3bdf");
    assert_eq!(domain_to_ascii("ابج").unwrap(), "xn--mgbcm");
    assert_eq!(domain_to_ascii("abc.ابج").unwrap(), "abc.xn--mgbcm");
    assert_eq!(domain_to_ascii("אבּג.ابج").unwrap(), "xn--kdb3bdf.xn--mgbcm");
    assert!(domain_to_ascii("àא").is_err())
}

#[test]
fn unicode_before_delimiter() {
    ffi::InitializeICUForTesting();
    assert!(domain_to_ascii("xn--f\u{34a}-PTP").is_err());
}
