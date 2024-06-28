// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::error::Error;

use crate::ffi;

impl ffi::SkusResult {
    pub fn new(code: ffi::SkusResultCode, msg: &str) -> Self {
        ffi::SkusResult {
            code,
            msg: msg.to_string(),
        }
    }
}

impl From<skus::errors::SkusError> for ffi::SkusResult {
    fn from(e: skus::errors::SkusError) -> Self {
        e.source()
            .expect("SkusError wraps an InternalError source")
            .downcast_ref::<skus::errors::InternalError>()
            .expect("SkusError wraps an InternalError source")
            .into()
    }
}

impl From<skus::errors::InternalError> for ffi::SkusResult {
    fn from(e: skus::errors::InternalError) -> Self {
        (&e).into()
    }
}

impl From<&skus::errors::InternalError> for ffi::SkusResult {
    fn from(e: &skus::errors::InternalError) -> Self {
        ffi::SkusResult {
            code: ffi::SkusResultCode::from(e),
            msg: e.to_string(),
        }
    }
}

impl From<&skus::errors::InternalError> for ffi::SkusResultCode {
    fn from(e: &skus::errors::InternalError) -> Self {
        match e {
            skus::errors::InternalError::RequestFailed => ffi::SkusResultCode::RequestFailed,
            skus::errors::InternalError::InternalServer(_) => ffi::SkusResultCode::InternalServer,
            skus::errors::InternalError::BadRequest(_) => ffi::SkusResultCode::BadRequest,
            skus::errors::InternalError::UnhandledStatus(_) => ffi::SkusResultCode::UnhandledStatus,
            skus::errors::InternalError::RetryLater(_) => ffi::SkusResultCode::RetryLater,
            skus::errors::InternalError::NotFound => ffi::SkusResultCode::NotFound,
            skus::errors::InternalError::SerializationFailed => {
                ffi::SkusResultCode::SerializationFailed
            }
            skus::errors::InternalError::InvalidResponse(_) => ffi::SkusResultCode::InvalidResponse,
            skus::errors::InternalError::InvalidProof => ffi::SkusResultCode::InvalidProof,
            skus::errors::InternalError::QueryError => ffi::SkusResultCode::QueryError,
            skus::errors::InternalError::OutOfCredentials => ffi::SkusResultCode::OutOfCredentials,
            skus::errors::InternalError::StorageWriteFailed(_) => {
                ffi::SkusResultCode::StorageWriteFailed
            }
            skus::errors::InternalError::StorageReadFailed(_) => ffi::SkusResultCode::StorageReadFailed,
            skus::errors::InternalError::OrderUnpaid => ffi::SkusResultCode::OrderUnpaid,
            skus::errors::InternalError::UnhandledVariant => ffi::SkusResultCode::UnhandledVariant,
            skus::errors::InternalError::OrderLocationMismatch => {
                ffi::SkusResultCode::OrderLocationMismatch
            }
            skus::errors::InternalError::OrderMisconfiguration => {
                ffi::SkusResultCode::OrderMisconfiguration
            }
            skus::errors::InternalError::ItemCredentialsMissing => {
                ffi::SkusResultCode::ItemCredentialsMissing
            }
            skus::errors::InternalError::ItemCredentialsExpired => {
                ffi::SkusResultCode::ItemCredentialsExpired
            }
            skus::errors::InternalError::InvalidMerchantOrSku => {
                ffi::SkusResultCode::InvalidMerchantOrSku
            }
            skus::errors::InternalError::BorrowFailed => ffi::SkusResultCode::BorrowFailed,
            skus::errors::InternalError::FutureCancelled => ffi::SkusResultCode::FutureCancelled,
            skus::errors::InternalError::InvalidCall(_) => ffi::SkusResultCode::InvalidCall,
        }
    }
}
