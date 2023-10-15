/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::str::Utf8Error;

use adblock::{blocker::BlockerError, resources::AddResourceError};
use thiserror::Error;

use crate::engine::Engine;
use crate::ffi::*;

#[derive(Debug, Error)]
pub(crate) enum InternalError {
    #[error("json error: {0}")]
    Json(#[from] serde_json::Error),
    #[error("utf-8 encoding error: {0}")]
    Utf8(#[from] Utf8Error),
    #[error("{0}")]
    Blocker(#[from] BlockerError),
    #[error("{0}")]
    AddResource(#[from] AddResourceError),
}

impl From<&InternalError> for ResultKind {
    fn from(error: &InternalError) -> Self {
        match error {
            InternalError::Json(_) => Self::JsonError,
            InternalError::Utf8(_) => Self::Utf8Error,
            InternalError::Blocker(_) | InternalError::AddResource(_) => Self::AdblockError,
        }
    }
}

/// This is an intermediate "result" type.
///
/// The purpose of this is to streamline conversions of typical Rust results to
/// final cxx-compatible structures that are to be consumed by callers.
/// The PreResult is generic to make conversions from Rust results easy
/// (via implementation of the From<Result<T, InternalError>> trait). Since cxx
/// does not support generics, this PreResult to converted to a similar final
/// structure that is not generic.
///
/// The typical chain of conversion for "result" types is as follows:
/// Result<T, InternalError> to PreResult<T> to
/// TResult (i.e. StringResult defined in the ffi module).
struct PreResult<T: Default> {
    value: T,
    result_kind: ResultKind,
    error_message: String,
}

/// Implements the From<PreResult<T>> trait for a given final ffi result type.
macro_rules! impl_result_from_trait {
    ($result_type:ty, $value_type:ty) => {
        impl From<Result<$value_type, InternalError>> for $result_type {
            fn from(result: Result<$value_type, InternalError>) -> Self {
                let PreResult { value, result_kind, error_message } = PreResult::from(result);
                Self { value, result_kind, error_message }
            }
        }
    };
}

impl<T> From<Result<T, InternalError>> for PreResult<T>
where
    T: Default,
{
    fn from(result: Result<T, InternalError>) -> Self {
        match result {
            Ok(value) => {
                Self { value, result_kind: ResultKind::Success, error_message: String::new() }
            }
            Err(e) => Self {
                value: Default::default(),
                result_kind: (&e).into(),
                error_message: e.to_string(),
            },
        }
    }
}

impl_result_from_trait!(VecStringResult, Vec<String>);
impl_result_from_trait!(BoxEngineResult, Box<Engine>);
#[cfg(feature = "ios")]
impl_result_from_trait!(ContentBlockingRulesResult, ContentBlockingRules);

/// `cxx` doesn't support unit values in structs,
/// so we have a separate From<_> implementation for the UnitResult.
impl From<Result<(), InternalError>> for UnitResult {
    fn from(result: Result<(), InternalError>) -> Self {
        let PreResult { result_kind, error_message, .. } = PreResult::from(result);
        Self { result_kind, error_message }
    }
}
