mod credentials;
mod orders;

pub use credentials::*;
pub use orders::*;

use std::cell::RefCell;
use std::error::Error;
use std::fmt;

use git_version::git_version;
use tracing::{event, Level};

use crate::cache::CacheNode;
use crate::models::*;
use crate::{HTTPClient, StorageClient};

pub struct SDK<U> {
    pub client: U,
    pub environment: Environment,
    pub base_url: String,
    pub remote_sdk_url: String,
    pub cache: RefCell<CacheNode<http::Response<Vec<u8>>>>,
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
        let base_url = base_url.unwrap_or_else(|| match environment {
            Environment::Local => "http://localhost:3333",
            Environment::Testing => "https://payment.rewards.brave.software",
            Environment::Development => "https://payment.rewards.brave.software",
            Environment::Staging => "https://payment.rewards.bravesoftware.com",
            Environment::Production => "https://payment.rewards.brave.com",
        });

        let remote_sdk_url = remote_sdk_url.unwrap_or_else(|| match environment {
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
        }
    }

    pub async fn initialize(&self) -> Result<(), Box<dyn Error>> {
        if self.environment == Environment::Local {
            self.client.clear().await?;
        }
        event!(
            Level::INFO,
            environment = %self.environment,
            version = %git_version!(),
            "skus sdk initialized",
        );

        Ok(())
    }
}
