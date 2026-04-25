// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::str::FromStr;

#[derive(Debug)]
pub enum Error {
    Cid(cid::Error),
    AlreadyUnwrapped,
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::Cid(e) => write!(f, "{}", e),
            Error::AlreadyUnwrapped => write!(f, "Result was already unwrapped"),
        }
    }
}

impl std::error::Error for Error {}

impl_error!(cid::Error, Cid);

#[macro_export]
macro_rules! impl_result {
    ($t:ident, $r:ident, $f:ident) => {
        impl $r {
            fn error_message(self: &$r) -> String {
                match &self.0 {
                    Err(e) => e.to_string(),
                    Ok(_) => String::new(),
                }
            }

            fn is_ok(self: &$r) -> bool {
                self.0.is_ok()
            }

            fn unwrap(self: &mut $r) -> Box<$t> {
                match std::mem::replace(&mut self.0, Err(Error::AlreadyUnwrapped)) {
                    Ok(v) => Box::new(v),
                    Err(e) => panic!("{}", e.to_string()),
                }
            }
        }

        impl From<Result<$f, Error>> for $r {
            fn from(result: Result<$f, Error>) -> Self {
                match result {
                    Ok(v) => Self(Ok($t(v))),
                    Err(e) => Self(Err(e)),
                }
            }
        }
    };
}

#[macro_export]
macro_rules! impl_error {
    ($t:ty, $n:ident) => {
        impl From<$t> for Error {
            fn from(err: $t) -> Self {
                Self::$n(err)
            }
        }
    };
}

pub struct EnsuredCid(String);
pub struct EnsureCidResult(Result<EnsuredCid, Error>);
impl_result!(EnsuredCid, EnsureCidResult, String);

impl EnsuredCid {
    pub fn value(&self) -> String {
        self.0.clone()
    }
}

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = filecoin)]
mod ffi {
    extern "Rust" {
        type EnsuredCid;
        type EnsureCidResult;

        fn is_valid_cid(cid: &str) -> bool;
        fn ensure_cidv1(cid_str: &str) -> Box<EnsureCidResult>;

        // Accessor methods for EnsureCidResult
        fn unwrap(self: &mut EnsureCidResult) -> Box<EnsuredCid>;
        fn error_message(self: &EnsureCidResult) -> String;
        fn is_ok(self: &EnsureCidResult) -> bool;

        // Accessor methods for EnsuredCid
        fn value(self: &EnsuredCid) -> String;
    }
}

pub fn is_valid_cid(cid_str: &str) -> bool {
    return cid::Cid::from_str(cid_str).is_ok();
}

pub fn ensure_cidv1(cid_str: &str) -> Box<EnsureCidResult> {
    Box::new(EnsureCidResult::from(ensure_cidv1_impl(cid_str)))
}

fn ensure_cidv1_impl(cid_str: &str) -> Result<String, Error> {
    let cid = cid_str.parse::<cid::Cid>()?;

    if cid.version() == cid::Version::V1 {
        Ok(cid_str.to_string())
    } else {
        Ok(cid.into_v1()?.to_string())
    }
}
