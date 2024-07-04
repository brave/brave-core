use core::fmt;
use core::fmt::Display;

use http::StatusCode;
use std::error::Error;
use std::time::Duration;

use crate::models::APIError;

use serde_json::Value;

/// Internal errors.  Most application-level developers will likely not
/// need to pay any attention to these.
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum InternalError {
    RequestFailed,
    InternalServer(APIError),
    BadRequest(APIError),
    UnhandledStatus(APIError),
    RetryLater(Option<Duration>),
    NotFound,
    SerializationFailed,
    InvalidResponse(String),
    InvalidProof,
    QueryError,
    OutOfCredentials,
    StorageWriteFailed(String),
    StorageReadFailed(String),
    OrderUnpaid,
    OrderMisconfiguration,
    UnhandledVariant,
    OrderLocationMismatch,
    ItemCredentialsMissing,
    ItemCredentialsExpired,
    InvalidMerchantOrSku,
    BorrowFailed,
    FutureCancelled,
    InvalidCall(String),
}

impl Display for InternalError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            InternalError::RequestFailed => write!(f, "HTTP request failed"),
            InternalError::InternalServer(app_err) => {
                let code: StatusCode =
                    StatusCode::from_u16(app_err.code).unwrap_or(StatusCode::INTERNAL_SERVER_ERROR);
                write!(
                    f,
                    "Server internal error: {} {} - {} {}",
                    code.as_str(),
                    code.canonical_reason().unwrap_or("unknown"),
                    app_err.error_code,
                    app_err.message,
                )
            }
            InternalError::BadRequest(app_err) => {
                let code: StatusCode =
                    StatusCode::from_u16(app_err.code).unwrap_or(StatusCode::BAD_REQUEST);
                write!(
                    f,
                    "Bad client request: {} {} - {} {} {}",
                    code.as_str(),
                    code.canonical_reason().unwrap_or("unknown"),
                    app_err.error_code,
                    app_err.message,
                    app_err.data.get("validationErrors").unwrap_or(&Value::Null),
                )
            }
            InternalError::UnhandledStatus(app_err) => {
                let code: StatusCode =
                    StatusCode::from_u16(app_err.code).unwrap_or(StatusCode::INTERNAL_SERVER_ERROR);
                write!(
                    f,
                    "Unhandled request status: {} {} - {} {}",
                    code.as_str(),
                    code.canonical_reason().unwrap_or("unknown"),
                    app_err.error_code,
                    app_err.message,
                )
            }
            InternalError::RetryLater(after) => write!(
                f,
                "Retry later{}",
                after.map(|a| format!("after {} ms", a.as_millis())).unwrap_or_default()
            ),
            InternalError::NotFound => write!(f, "Resource not found"),
            InternalError::SerializationFailed => write!(f, "Could not (de)serialize"),
            InternalError::InvalidResponse(reason) => {
                write!(f, "Server returned an invalid response: {}", reason)
            }
            InternalError::InvalidProof => write!(f, "Included proof was not valid"),
            InternalError::QueryError => write!(f, "Error in the query"),
            InternalError::OutOfCredentials => write!(f, "All credentials have been spent"),
            InternalError::StorageWriteFailed(reason) => {
                write!(f, "Failed to write changes to storage {}", reason)
            }
            InternalError::StorageReadFailed(reason) => {
                write!(f, "Failed to read from storage: {}", reason)
            }
            InternalError::OrderUnpaid => write!(f, "The order is unpaid"),
            InternalError::OrderMisconfiguration => write!(f, "The order is misconfigured"),
            InternalError::UnhandledVariant => write!(f, "Variant is unhandled"),
            InternalError::OrderLocationMismatch => write!(f, "Order location does not match"),
            InternalError::ItemCredentialsMissing => {
                write!(f, "Item credentials are missing or incomplete")
            }
            InternalError::ItemCredentialsExpired => {
                write!(f, "Item credentials have expired")
            }
            InternalError::InvalidMerchantOrSku => {
                write!(f, "Invalid merchant or sku")
            }
            InternalError::BorrowFailed => {
                write!(f, "Borrow failed")
            }
            InternalError::FutureCancelled => {
                write!(f, "Future was cancelled")
            }
            InternalError::InvalidCall(reason) => {
                write!(f, "Caller did not follow required call convention: {}", reason)
            }
        }
    }
}

impl Error for InternalError {}

impl From<(InternalError, usize)> for InternalError {
    fn from((e, _attempt): (InternalError, usize)) -> Self {
        e
    }
}

impl From<serde_json::Error> for InternalError {
    fn from(_: serde_json::Error) -> Self {
        InternalError::SerializationFailed
    }
}

impl From<http::Error> for InternalError {
    fn from(e: http::Error) -> Self {
        InternalError::InvalidCall(e.to_string())
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct SkusError(pub(crate) InternalError);

impl Display for SkusError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl Error for SkusError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        Some(&self.0)
    }
}

impl From<InternalError> for SkusError {
    fn from(e: InternalError) -> Self {
        SkusError(e)
    }
}

impl From<(InternalError, usize)> for SkusError {
    fn from((e, _attempt): (InternalError, usize)) -> Self {
        SkusError(e)
    }
}

impl From<serde_json::Error> for SkusError {
    fn from(e: serde_json::Error) -> Self {
        SkusError(e.into())
    }
}

pub trait DebugUnwrap {
    fn debug_unwrap(self) -> Self;
}

impl<T> DebugUnwrap for Result<T, InternalError> {
    fn debug_unwrap(self) -> Self {
        self.map_err(|e| {
            debug_assert!(false, "debug_unwrap called on Result::Err: {}", e);
            e
        })
    }
}
