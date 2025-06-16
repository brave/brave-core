mod credentials;
mod orders;

use std::cell::RefCell;
use std::collections::VecDeque;
use std::fmt;
use std::sync::Arc;
use std::sync::Mutex;

use tracing::{event, Level};
#[cfg(not(test))]
use tracing_subscriber::fmt::format::DefaultFields;

use crate::cache::CacheNode;
#[cfg(not(test))]
use crate::log::RingBufferLayer;
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
    pub log_buffer: Option<Arc<Mutex<VecDeque<char>>>>,
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
            log_buffer: None,
        }
    }

    #[cfg(not(test))]
    pub fn create_log_buffer_layer(
        &mut self,
        main_capacity: usize,
        span_capacity: usize,
    ) -> RingBufferLayer<DefaultFields> {
        let buf = Arc::new(Mutex::new(VecDeque::new()));
        self.log_buffer = Some(buf.clone());
        RingBufferLayer::new(buf, DefaultFields::new(), main_capacity, span_capacity)
    }

    pub fn get_logs(&self) -> String {
        if let Some(buf) = &self.log_buffer {
            if let Ok(buf) = buf.lock() {
                return buf.iter().collect();
            }
        }
        "".to_string()
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
}
