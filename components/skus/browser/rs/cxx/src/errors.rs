use crate::ffi;

impl From<&brave_rewards::errors::InternalError> for ffi::RewardsResult {
    fn from(e: &brave_rewards::errors::InternalError) -> Self {
        match e {
            brave_rewards::errors::InternalError::RequestFailed => {
                ffi::RewardsResult::RequestFailed
            }
            brave_rewards::errors::InternalError::InternalServer(_) => {
                ffi::RewardsResult::InternalServer
            }
            brave_rewards::errors::InternalError::BadRequest(_) => ffi::RewardsResult::BadRequest,
            brave_rewards::errors::InternalError::UnhandledStatus(_) => {
                ffi::RewardsResult::UnhandledStatus
            }
            brave_rewards::errors::InternalError::RetryLater(_) => ffi::RewardsResult::RetryLater,
            brave_rewards::errors::InternalError::NotFound => ffi::RewardsResult::NotFound,
            brave_rewards::errors::InternalError::SerializationFailed => {
                ffi::RewardsResult::SerializationFailed
            }
            brave_rewards::errors::InternalError::InvalidResponse(_) => {
                ffi::RewardsResult::InvalidResponse
            }
            brave_rewards::errors::InternalError::InvalidProof => ffi::RewardsResult::InvalidProof,
            brave_rewards::errors::InternalError::QueryError => ffi::RewardsResult::QueryError,
            brave_rewards::errors::InternalError::OutOfCredentials => {
                ffi::RewardsResult::OutOfCredentials
            }
            brave_rewards::errors::InternalError::StorageWriteFailed(_) => {
                ffi::RewardsResult::StorageWriteFailed
            }
            brave_rewards::errors::InternalError::StorageReadFailed(_) => {
                ffi::RewardsResult::StorageReadFailed
            }
            brave_rewards::errors::InternalError::OrderUnpaid => ffi::RewardsResult::OrderUnpaid,
            brave_rewards::errors::InternalError::UnhandledVariant => {
                ffi::RewardsResult::UnhandledVariant
            }
            brave_rewards::errors::InternalError::OrderLocationMismatch => {
                ffi::RewardsResult::OrderLocationMismatch
            }
            brave_rewards::errors::InternalError::ItemCredentialsMissing => {
                ffi::RewardsResult::ItemCredentialsMissing
            }
            brave_rewards::errors::InternalError::ItemCredentialsExpired => {
                ffi::RewardsResult::ItemCredentialsExpired
            }
            brave_rewards::errors::InternalError::InvalidMerchantOrSku => {
                ffi::RewardsResult::InvalidMerchantOrSku
            }
            brave_rewards::errors::InternalError::BorrowFailed => ffi::RewardsResult::BorrowFailed,
        }
    }
}

impl From<ffi::RewardsResult> for brave_rewards::errors::InternalError {
    fn from(e: ffi::RewardsResult) -> Self {
        match e {
            ffi::RewardsResult::RequestFailed => {
                brave_rewards::errors::InternalError::RequestFailed
            }
            ffi::RewardsResult::InternalServer => {
                brave_rewards::errors::InternalError::InternalServer(
                    brave_rewards::http::http::status::StatusCode::from_u16(599).unwrap(),
                )
            }
            ffi::RewardsResult::BadRequest => brave_rewards::errors::InternalError::BadRequest(
                brave_rewards::http::http::StatusCode::from_u16(499).unwrap(),
            ),
            ffi::RewardsResult::UnhandledStatus => {
                brave_rewards::errors::InternalError::UnhandledStatus(
                    brave_rewards::http::http::StatusCode::from_u16(699).unwrap(),
                )
            }
            ffi::RewardsResult::RetryLater => {
                brave_rewards::errors::InternalError::RetryLater(None)
            }
            ffi::RewardsResult::NotFound => brave_rewards::errors::InternalError::NotFound,
            ffi::RewardsResult::SerializationFailed => {
                brave_rewards::errors::InternalError::SerializationFailed
            }
            ffi::RewardsResult::InvalidResponse => {
                brave_rewards::errors::InternalError::InvalidResponse("".to_string())
            }
            ffi::RewardsResult::InvalidProof => brave_rewards::errors::InternalError::InvalidProof,
            ffi::RewardsResult::QueryError => brave_rewards::errors::InternalError::QueryError,
            ffi::RewardsResult::OutOfCredentials => {
                brave_rewards::errors::InternalError::OutOfCredentials
            }
            ffi::RewardsResult::StorageWriteFailed => {
                brave_rewards::errors::InternalError::StorageWriteFailed("".to_string())
            }
            ffi::RewardsResult::StorageReadFailed => {
                brave_rewards::errors::InternalError::StorageReadFailed("".to_string())
            }
            ffi::RewardsResult::OrderUnpaid => brave_rewards::errors::InternalError::OrderUnpaid,
            ffi::RewardsResult::UnhandledVariant => {
                brave_rewards::errors::InternalError::UnhandledVariant
            }
            ffi::RewardsResult::OrderLocationMismatch => {
                brave_rewards::errors::InternalError::OrderLocationMismatch
            }
            ffi::RewardsResult::ItemCredentialsMissing => {
                brave_rewards::errors::InternalError::ItemCredentialsMissing
            }
            ffi::RewardsResult::ItemCredentialsExpired => {
                brave_rewards::errors::InternalError::ItemCredentialsExpired
            }
            ffi::RewardsResult::InvalidMerchantOrSku => {
                brave_rewards::errors::InternalError::InvalidMerchantOrSku
            }
            ffi::RewardsResult::BorrowFailed => brave_rewards::errors::InternalError::BorrowFailed,
            _ => brave_rewards::errors::InternalError::UnhandledVariant,
        }
    }
}
