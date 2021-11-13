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
use crate::{HTTPClient, StorageClient};

pub struct SDK<U> {
    pub client: U,
    pub environment: String,
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
        environment: &str,
        base_url: Option<&str>,
        remote_sdk_url: Option<&str>,
    ) -> Self {
        let base_url = base_url.unwrap_or_else(|| match environment {
            // FIXME enum for environments
            "local" => "http://localhost:3333",
            "testing" => "https://payment.rewards.brave.software",
            "development" => "https://payment.rewards.brave.software",
            "staging" => "https://payment.rewards.bravesoftware.com",
            "production" => "https://payment.rewards.brave.com",
            _ => "",
        });

        let remote_sdk_url = remote_sdk_url.unwrap_or_else(|| match environment {
            // FIXME enum for environments
            "local" => "http://localhost:8080",
            "testing" => "https://account.brave.software/skus",
            "development" => "https://account.brave.software/skus",
            "staging" => "https://account.bravesoftware.com/skus",
            "production" => "https://account.brave.com/skus",
            _ => "",
        });

        SDK {
            client,
            environment: environment.to_string(),
            base_url: base_url.to_string(),
            remote_sdk_url: remote_sdk_url.to_string(),
            cache: RefCell::new(CacheNode::new()),
        }
    }

    pub async fn initialize(&self) -> Result<(), Box<dyn Error>> {
        if self.environment == "local" {
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
