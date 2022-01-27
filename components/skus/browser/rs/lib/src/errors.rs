use core::fmt;
use core::fmt::Display;

use std::error::Error;
use std::time::Duration;

/// Internal errors.  Most application-level developers will likely not
/// need to pay any attention to these.
#[derive(Clone, Debug, Eq, PartialEq, Hash)]
pub enum InternalError {
    RequestFailed,
    InternalServer(http::StatusCode),
    BadRequest(http::StatusCode),
    UnhandledStatus(http::StatusCode),
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
            InternalError::InternalServer(status) => {
                write!(
                    f,
                    "Server internal error: {} {}",
                    status.as_str(),
                    status.canonical_reason().unwrap_or("unknown")
                )
            }
            InternalError::BadRequest(status) => {
                write!(
                    f,
                    "Bad client request: {} {}",
                    status.as_str(),
                    status.canonical_reason().unwrap_or("unknown")
                )
            }
            InternalError::UnhandledStatus(status) => {
                write!(
                    f,
                    "Unhandled request status: {} {}",
                    status.as_str(),
                    status.canonical_reason().unwrap_or("unknown")
                )
            }
            InternalError::RetryLater(after) => write!(
                f,
                "Retry later{}",
                after
                    .map(|a| format!("after {} ms", a.as_millis()))
                    .unwrap_or_else(|| "".to_string())
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
                write!(
                    f,
                    "Caller did not follow required call convention: {}",
                    reason
                )
            }
        }
    }
}

impl Error for InternalError {}

impl From<serde_json::Error> for InternalError {
    fn from(_: serde_json::Error) -> Self {
        InternalError::SerializationFailed
    }
}

#[derive(Clone, Debug, Eq, PartialEq, Hash)]
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
