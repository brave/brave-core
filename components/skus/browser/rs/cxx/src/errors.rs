use crate::ffi;

impl From<&skus::errors::InternalError> for ffi::RewardsResult {
    fn from(e: &skus::errors::InternalError) -> Self {
        match e {
            skus::errors::InternalError::RequestFailed => ffi::RewardsResult::RequestFailed,
            skus::errors::InternalError::InternalServer(_) => ffi::RewardsResult::InternalServer,
            skus::errors::InternalError::BadRequest(_) => ffi::RewardsResult::BadRequest,
            skus::errors::InternalError::UnhandledStatus(_) => ffi::RewardsResult::UnhandledStatus,
            skus::errors::InternalError::RetryLater(_) => ffi::RewardsResult::RetryLater,
            skus::errors::InternalError::NotFound => ffi::RewardsResult::NotFound,
            skus::errors::InternalError::SerializationFailed => {
                ffi::RewardsResult::SerializationFailed
            }
            skus::errors::InternalError::InvalidResponse(_) => ffi::RewardsResult::InvalidResponse,
            skus::errors::InternalError::InvalidProof => ffi::RewardsResult::InvalidProof,
            skus::errors::InternalError::QueryError => ffi::RewardsResult::QueryError,
            skus::errors::InternalError::OutOfCredentials => ffi::RewardsResult::OutOfCredentials,
            skus::errors::InternalError::StorageWriteFailed(_) => {
                ffi::RewardsResult::StorageWriteFailed
            }
            skus::errors::InternalError::StorageReadFailed(_) => {
                ffi::RewardsResult::StorageReadFailed
            }
            skus::errors::InternalError::OrderUnpaid => ffi::RewardsResult::OrderUnpaid,
            skus::errors::InternalError::UnhandledVariant => ffi::RewardsResult::UnhandledVariant,
            skus::errors::InternalError::OrderLocationMismatch => {
                ffi::RewardsResult::OrderLocationMismatch
            }
            skus::errors::InternalError::ItemCredentialsMissing => {
                ffi::RewardsResult::ItemCredentialsMissing
            }
            skus::errors::InternalError::ItemCredentialsExpired => {
                ffi::RewardsResult::ItemCredentialsExpired
            }
            skus::errors::InternalError::InvalidMerchantOrSku => {
                ffi::RewardsResult::InvalidMerchantOrSku
            }
            skus::errors::InternalError::BorrowFailed => ffi::RewardsResult::BorrowFailed,
        }
    }
}

impl From<ffi::RewardsResult> for skus::errors::InternalError {
    fn from(e: ffi::RewardsResult) -> Self {
        match e {
            ffi::RewardsResult::RequestFailed => skus::errors::InternalError::RequestFailed,
            ffi::RewardsResult::InternalServer => skus::errors::InternalError::InternalServer(
                skus::http::http::status::StatusCode::from_u16(599).unwrap(),
            ),
            ffi::RewardsResult::BadRequest => skus::errors::InternalError::BadRequest(
                skus::http::http::StatusCode::from_u16(499).unwrap(),
            ),
            ffi::RewardsResult::UnhandledStatus => skus::errors::InternalError::UnhandledStatus(
                skus::http::http::StatusCode::from_u16(699).unwrap(),
            ),
            ffi::RewardsResult::RetryLater => skus::errors::InternalError::RetryLater(None),
            ffi::RewardsResult::NotFound => skus::errors::InternalError::NotFound,
            ffi::RewardsResult::SerializationFailed => {
                skus::errors::InternalError::SerializationFailed
            }
            ffi::RewardsResult::InvalidResponse => {
                skus::errors::InternalError::InvalidResponse("".to_string())
            }
            ffi::RewardsResult::InvalidProof => skus::errors::InternalError::InvalidProof,
            ffi::RewardsResult::QueryError => skus::errors::InternalError::QueryError,
            ffi::RewardsResult::OutOfCredentials => skus::errors::InternalError::OutOfCredentials,
            ffi::RewardsResult::StorageWriteFailed => {
                skus::errors::InternalError::StorageWriteFailed("".to_string())
            }
            ffi::RewardsResult::StorageReadFailed => {
                skus::errors::InternalError::StorageReadFailed("".to_string())
            }
            ffi::RewardsResult::OrderUnpaid => skus::errors::InternalError::OrderUnpaid,
            ffi::RewardsResult::UnhandledVariant => skus::errors::InternalError::UnhandledVariant,
            ffi::RewardsResult::OrderLocationMismatch => {
                skus::errors::InternalError::OrderLocationMismatch
            }
            ffi::RewardsResult::ItemCredentialsMissing => {
                skus::errors::InternalError::ItemCredentialsMissing
            }
            ffi::RewardsResult::ItemCredentialsExpired => {
                skus::errors::InternalError::ItemCredentialsExpired
            }
            ffi::RewardsResult::InvalidMerchantOrSku => {
                skus::errors::InternalError::InvalidMerchantOrSku
            }
            ffi::RewardsResult::BorrowFailed => skus::errors::InternalError::BorrowFailed,
            _ => skus::errors::InternalError::UnhandledVariant,
        }
    }
}
