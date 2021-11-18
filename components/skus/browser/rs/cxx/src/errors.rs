use std::error::Error;

use crate::ffi;

impl From<&skus::errors::InternalError> for ffi::SkusResult {
    fn from(e: &skus::errors::InternalError) -> Self {
        match e {
            skus::errors::InternalError::RequestFailed => ffi::SkusResult::RequestFailed,
            skus::errors::InternalError::InternalServer(_) => ffi::SkusResult::InternalServer,
            skus::errors::InternalError::BadRequest(_) => ffi::SkusResult::BadRequest,
            skus::errors::InternalError::UnhandledStatus(_) => ffi::SkusResult::UnhandledStatus,
            skus::errors::InternalError::RetryLater(_) => ffi::SkusResult::RetryLater,
            skus::errors::InternalError::NotFound => ffi::SkusResult::NotFound,
            skus::errors::InternalError::SerializationFailed => {
                ffi::SkusResult::SerializationFailed
            }
            skus::errors::InternalError::InvalidResponse(_) => ffi::SkusResult::InvalidResponse,
            skus::errors::InternalError::InvalidProof => ffi::SkusResult::InvalidProof,
            skus::errors::InternalError::QueryError => ffi::SkusResult::QueryError,
            skus::errors::InternalError::OutOfCredentials => ffi::SkusResult::OutOfCredentials,
            skus::errors::InternalError::StorageWriteFailed(_) => {
                ffi::SkusResult::StorageWriteFailed
            }
            skus::errors::InternalError::StorageReadFailed(_) => ffi::SkusResult::StorageReadFailed,
            skus::errors::InternalError::OrderUnpaid => ffi::SkusResult::OrderUnpaid,
            skus::errors::InternalError::UnhandledVariant => ffi::SkusResult::UnhandledVariant,
            skus::errors::InternalError::OrderLocationMismatch => {
                ffi::SkusResult::OrderLocationMismatch
            }
            skus::errors::InternalError::ItemCredentialsMissing => {
                ffi::SkusResult::ItemCredentialsMissing
            }
            skus::errors::InternalError::ItemCredentialsExpired => {
                ffi::SkusResult::ItemCredentialsExpired
            }
            skus::errors::InternalError::InvalidMerchantOrSku => {
                ffi::SkusResult::InvalidMerchantOrSku
            }
            skus::errors::InternalError::BorrowFailed => ffi::SkusResult::BorrowFailed,
            skus::errors::InternalError::FutureCancelled => ffi::SkusResult::FutureCancelled,
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

impl From<ffi::SkusResult> for skus::errors::InternalError {
    fn from(e: ffi::SkusResult) -> Self {
        match e {
            ffi::SkusResult::RequestFailed => skus::errors::InternalError::RequestFailed,
            ffi::SkusResult::InternalServer => skus::errors::InternalError::InternalServer(
                skus::http::http::status::StatusCode::from_u16(599).expect("100 < 599 < 1000"),
            ),
            ffi::SkusResult::BadRequest => skus::errors::InternalError::BadRequest(
                skus::http::http::StatusCode::from_u16(499).expect("100 < 499 < 1000"),
            ),
            ffi::SkusResult::UnhandledStatus => skus::errors::InternalError::UnhandledStatus(
                skus::http::http::StatusCode::from_u16(699).expect("100 < 699 < 1000"),
            ),
            ffi::SkusResult::RetryLater => skus::errors::InternalError::RetryLater(None),
            ffi::SkusResult::NotFound => skus::errors::InternalError::NotFound,
            ffi::SkusResult::SerializationFailed => {
                skus::errors::InternalError::SerializationFailed
            }
            ffi::SkusResult::InvalidResponse => {
                skus::errors::InternalError::InvalidResponse("".to_string())
            }
            ffi::SkusResult::InvalidProof => skus::errors::InternalError::InvalidProof,
            ffi::SkusResult::QueryError => skus::errors::InternalError::QueryError,
            ffi::SkusResult::OutOfCredentials => skus::errors::InternalError::OutOfCredentials,
            ffi::SkusResult::StorageWriteFailed => {
                skus::errors::InternalError::StorageWriteFailed("".to_string())
            }
            ffi::SkusResult::StorageReadFailed => {
                skus::errors::InternalError::StorageReadFailed("".to_string())
            }
            ffi::SkusResult::OrderUnpaid => skus::errors::InternalError::OrderUnpaid,
            ffi::SkusResult::UnhandledVariant => skus::errors::InternalError::UnhandledVariant,
            ffi::SkusResult::OrderLocationMismatch => {
                skus::errors::InternalError::OrderLocationMismatch
            }
            ffi::SkusResult::ItemCredentialsMissing => {
                skus::errors::InternalError::ItemCredentialsMissing
            }
            ffi::SkusResult::ItemCredentialsExpired => {
                skus::errors::InternalError::ItemCredentialsExpired
            }
            ffi::SkusResult::InvalidMerchantOrSku => {
                skus::errors::InternalError::InvalidMerchantOrSku
            }
            ffi::SkusResult::BorrowFailed => skus::errors::InternalError::BorrowFailed,
            ffi::SkusResult::FutureCancelled => skus::errors::InternalError::FutureCancelled,
            _ => skus::errors::InternalError::UnhandledVariant,
        }
    }
}
