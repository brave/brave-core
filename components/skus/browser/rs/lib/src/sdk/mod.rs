mod credentials;
mod orders;

pub use credentials::*;
pub use orders::*;

use std::cell::RefCell;
use std::fmt;

use chrono::{DateTime, Utc};
#[cfg(feature = "wasm")]
use git_version::git_version;
use tracing::{event, Level};

use crate::cache::CacheNode;
use crate::models::*;
use crate::{HTTPClient, StorageClient};

#[cfg(feature = "wasm")]
const VERSION: &str = git_version!();
#[cfg(not(feature = "wasm"))]
const VERSION: &str = "unknown";

pub struct SDK<U> {
    pub client: U,
    pub environment: Environment,
    pub base_url: String,
    pub remote_sdk_url: String,
    pub cache: RefCell<CacheNode<http::Response<Vec<u8>>>>,
    #[cfg(test)]
    pub now: DateTime<Utc>,
}

impl<U> fmt::Debug for SDK<U> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("SDK").finish()
    }
}

impl<U> SDK<U>
where
    U: HTTPClient + StorageClient,
{
    pub fn new(
        client: U,
        environment: Environment,
        base_url: Option<&str>,
        remote_sdk_url: Option<&str>,
    ) -> Self {
        let base_url = base_url.unwrap_or(match environment {
            Environment::Local => "http://localhost:3333",
            Environment::Testing => "https://payment.rewards.brave.software",
            Environment::Development => "https://payment.rewards.brave.software",
            Environment::Staging => "https://payment.rewards.bravesoftware.com",
            Environment::Production => "https://payment.rewards.brave.com",
        });

        let remote_sdk_url = remote_sdk_url.unwrap_or(match environment {
            Environment::Local => "http://localhost:8080",
            Environment::Testing => "https://account.brave.software/skus",
            Environment::Development => "https://account.brave.software/skus",
            Environment::Staging => "https://account.bravesoftware.com/skus",
            Environment::Production => "https://account.brave.com/skus",
        });

        SDK {
            client,
            environment,
            base_url: base_url.to_string(),
            remote_sdk_url: remote_sdk_url.to_string(),
            cache: RefCell::new(CacheNode::default()),
            #[cfg(test)]
            now: Utc::now(),
        }
    }

    pub async fn initialize(&self) {
        if self.environment == Environment::Local && self.client.clear().await.is_err() {
            event!(Level::ERROR, "clearing skus storage failed",);
        }
        event!(
            Level::INFO,
            environment = %self.environment,
            version = VERSION,
            "skus sdk initialized",
        );
    }

    #[cfg(not(test))]
    pub fn now(&self) -> DateTime<Utc> {
        Utc::now()
    }

    #[cfg(test)]
    pub fn now(&self) -> DateTime<Utc> {
        self.now
    }
}

#[cfg(test)]
mod tests {
    use crate::errors::InternalError;
    use crate::http::http::Response;
    use crate::models::*;
    use crate::{HTTPClient, StorageClient};
    use challenge_bypass_ristretto::voprf::*;

    use async_trait::async_trait;

    use mockall::mock;

    pub static SAMPLE_ORDER_ID: &'static str = "db7ee095-8a05-4013-90cb-53e6e9f3a54e";
    pub static SAMPLE_ITEM_ID: &'static str = "7ebcab08-8bbe-46eb-b263-ab68bd2eaf2e";
    pub static SAMPLE_ITEM_DOMAIN: &'static str = "vpn.brave.com";
    pub static SAMPLE_TIME_LIMITED_V2_CREDENTIAL_VALID_NANOS: i64 = 1683849600000000000;
    pub static SAMPLE_TIME_LIMITED_V2_CREDENTIAL_LAST_VALID_NANOS: i64 = 1684281600000000000;
    pub static SAMPLE_TIME_LIMITED_V2_CREDENTIALS: &'static str = r###"
    {
        "creds": [
        ],
        "item_id": "7ebcab08-8bbe-46eb-b263-ab68bd2eaf2e",
        "state": "GeneratedCredentials",
        "type": "time-limited-v2",
        "unblinded_creds": [
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-11T15:04:05",
                "valid_to": "2023-05-12T15:04:05"
            },
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-12T15:04:05",
                "valid_to": "2023-05-13T15:04:05"
            },
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-13T15:04:05",
                "valid_to": "2023-05-14T15:04:05"
            },
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-14T15:04:05",
                "valid_to": "2023-05-15T15:04:05"
            },
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-15T15:04:05",
                "valid_to": "2023-05-16T15:04:05"
            },
            {
                "issuer_id": "213b4359-fbec-45fa-b146-f1002d6a7b8b",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-05-16T15:04:05",
                "valid_to": "2023-05-17T15:04:05"
            }
        ]
    }
    "###;
    pub static SAMPLE_ORDER: &'static str = r###"
    {
        "created_at": "2023-05-10T15:04:05.000000",
        "currency": "USD",
        "expires_at": "2023-06-10T15:04:05.000000",
        "id": "db7ee095-8a05-4013-90cb-53e6e9f3a54e",
        "items": [
            {
                "created_at": "2023-05-10T15:04:05.000000",
                "credential_type": "time-limited-v2",
                "currency": "USD",
                "description": "brave-vpn-premium",
                "id": "7ebcab08-8bbe-46eb-b263-ab68bd2eaf2e",
                "location": "vpn.brave.com",
                "order_id": "db7ee095-8a05-4013-90cb-53e6e9f3a54e",
                "price": 9.99,
                "quantity": 1,
                "sku": "brave-vpn-premium",
                "subtotal": 9.99,
                "updated_at": "2023-05-10T15:04:05.000000"
            }
        ],
        "last_paid_at": "2023-05-10T15:04:05.000000",
        "location": "vpn.brave.com",
        "merchant_id": "brave.com",
        "metadata": {
            "num_intervals": 33,
            "num_per_interval": 2,
            "payment_processor": "stripe",
            "stripe_checkout_session_id": "cs_live_oQ+UzQPYE5tBbqUrnVKudv6AKuH9hlal2qf2F9tJhIgiWqar+2fvEV8Cog"
        },
        "status": "paid",
        "total_price": 9.99,
        "updated_at": "2023-05-10T15:04:05.000000"
    }
    "###;

    pub static SAMPLE_EXPIRED_ORDER_ID: &'static str = "c18f0635-10a1-4e29-89d0-428046278ce5";
    pub static SAMPLE_EXPIRED_ITEM_ID: &'static str = "6a4c1691-4fc4-482a-8cf1-e84d2e8493dc";
    pub static SAMPLE_EXPIRED_TIME_LIMITED_V2_CREDENTIALS: &'static str = r###"
    {
        "creds": [
        ],
        "item_id": "6a4c1691-4fc4-482a-8cf1-e84d2e8493dc",
        "state": "GeneratedCredentials",
        "type": "time-limited-v2",
        "unblinded_creds": [
            {
                "issuer_id": "350d49f9-1f94-4d4e-8e71-06a9044d0de4",

                "unblinded_creds": [
                    {
                    "spent": false,
                    "unblinded_cred": "lgGYje8K56vEOByP1FSivdzE2s97Ua7lX3jAozw6oZINFOPbz7RKKP9pHykgb6LwvDp2ZBjobP/lJSTKnFYTOyRSciOLVHY47bnfWuKmWpaeYvUGOCNMf1YQoVbuD+9e"
                    }
                ],
                "valid_from": "2023-04-16T15:04:05",
                "valid_to": "2023-04-17T15:04:05"
            }
        ]
    }
    "###;
    pub static SAMPLE_EXPIRED_ORDER: &'static str = r###"
    {
        "created_at": "2023-04-10T15:04:05.000000",
        "currency": "USD",
        "expires_at": "2023-05-10T15:04:05.000000",
        "id": "c18f0635-10a1-4e29-89d0-428046278ce5",
        "items": [
            {
                "created_at": "2023-05-10T15:04:05.000000",
                "credential_type": "time-limited-v2",
                "currency": "USD",
                "description": "brave-vpn-premium",
                "id": "6a4c1691-4fc4-482a-8cf1-e84d2e8493dc",
                "location": "vpn.brave.com",
                "order_id": "c18f0635-10a1-4e29-89d0-428046278ce5",
                "price": 9.99,
                "quantity": 1,
                "sku": "brave-vpn-premium",
                "subtotal": 9.99,
                "updated_at": "2023-04-10T15:04:05.000000"
            }
        ],
        "last_paid_at": "2023-04-10T15:04:05.000000",
        "location": "vpn.brave.com",
        "merchant_id": "brave.com",
        "metadata": {
            "num_intervals": 33,
            "num_per_interval": 2,
            "payment_processor": "stripe",
            "stripe_checkout_session_id": "cs_live_oQ+UzQPYE5tBbqUrnVKudv6AKuH9hlal2qf2F9tJhIgiWqar+2fvEV8Cog"
        },
        "status": "paid",
        "total_price": 9.99,
        "updated_at": "2023-04-10T15:04:05.000000"
    }
    "###;

    mock! {
        pub Client {}

        #[async_trait(?Send)]
        impl HTTPClient for Client {
            async fn execute(
                &self,
                req: http::Request<Vec<u8>>,
            ) -> Result<Response<Vec<u8>>, InternalError>;
            fn schedule_wakeup(&self, delay_ms: u64);
            fn get_cookie(&self, key: &str) -> Option<String>;
            fn set_cookie(&self, value: &str);
        }

        #[async_trait(?Send)]
        impl StorageClient for Client {
            async fn clear(&self) -> Result<(), InternalError>;
            async fn insert_wallet(&self, wallet: &Wallet) -> Result<(), InternalError>;
            async fn replace_promotions(&self, promotions: &[Promotion]) -> Result<(), InternalError>;
            async fn get_orders(&self) -> Result<Option<Vec<Order>>, InternalError>;
            async fn get_order(&self, order_id: &str) -> Result<Option<Order>, InternalError>;
            async fn has_credentials(&self, order_id: &str) -> Result<bool, InternalError>;
            async fn upsert_order(&self, order: &Order) -> Result<(), InternalError>;
            async fn delete_item_creds(&self, item_id: &str) -> Result<(), InternalError>;
            #[cfg(feature = "e2e_test")]
            async fn delete_n_item_creds(&self, item_id: &str, n: usize) -> Result<(), InternalError>;
            async fn get_time_limited_v2_creds(
                &self,
                item_id: &str,
            ) -> Result<Option<TimeLimitedV2Credentials>, InternalError>;
            async fn get_single_use_item_creds(
                &self,
                item_id: &str,
            ) -> Result<Option<SingleUseCredentials>, InternalError>;
            async fn init_single_use_item_creds(
                &self,
                item_id: &str,
                creds: Vec<Token>,
            ) -> Result<(), InternalError>;
            async fn upsert_time_limited_v2_item_creds(
                &self,
                item_id: &str,
                creds: Vec<Token>,
            ) -> Result<(), InternalError>;
            async fn append_time_limited_v2_item_unblinded_creds(
                &self,
                item_id: &str,
                issuer_id: &str,
                unblinded_creds: Vec<UnblindedToken>,
                valid_from: &str,
                valid_to: &str,
            ) -> Result<(), InternalError>;
            async fn complete_single_use_item_creds(
                &self,
                item_id: &str,
                issuer_id: &str,
                unblinded_creds: Vec<UnblindedToken>,
            ) -> Result<(), InternalError>;
            async fn spend_time_limited_v2_item_cred(
                &self,
                item_id: &str,
                issuer_id: &str,
                index: usize,
            ) -> Result<(), InternalError>;
            async fn spend_single_use_item_cred(
                &self,
                item_id: &str,
                index: usize,
            ) -> Result<(), InternalError>;
            async fn get_time_limited_creds(
                &self,
                item_id: &str,
            ) -> Result<Option<TimeLimitedCredentials>, InternalError>;
            async fn store_time_limited_creds(
                &self,
                item_id: &str,
                creds: Vec<TimeLimitedCredential>,
            ) -> Result<(), InternalError>;
        }
    }
}
