use std::cell::RefCell;
use std::cell::RefMut;
use std::collections::HashMap;
use std::fmt;
use std::io::Read;

use async_std::task;
use async_trait::async_trait;
use isahc::prelude::*;
use tracing::{debug, Level};
use tracing_subscriber::FmtSubscriber;

use skus::{errors::InternalError, Environment, HTTPClient, KVClient, KVStore};

pub struct CLIStore(HashMap<String, String>);

pub struct CLIClient {
    store: RefCell<CLIStore>,
}

impl fmt::Debug for CLIClient {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CLIClient").finish()
    }
}

#[async_trait(?Send)]
impl HTTPClient for CLIClient {
    fn get_cookie(&self, name: &str) -> Option<String> {
        debug!("Cookie get: {}", name);
        // FIXME
        None
    }

    fn set_cookie(&self, value: &str) {
        // FIXME
        debug!("Cookie set: {}", value);
    }

    fn schedule_wakeup(&self, delay_ms: u64) {
        // FIXME
        debug!("Schedule wakeup: {}", delay_ms);
    }

    async fn execute(
        &self,
        req: http::Request<Vec<u8>>,
    ) -> Result<http::Response<Vec<u8>>, InternalError> {
        let client = HttpClient::default();
        return match client.send_async(req).await {
            Ok(response) => Ok(response.map(|b| {
                let body: Result<Vec<u8>, _> = b.bytes().collect();
                body.unwrap()
            })),
            Err(_) => Err(InternalError::RequestFailed),
        };
    }
}

impl KVClient for CLIClient {
    type Store = CLIStore;

    fn get_store<'a>(&'a self) -> Result<RefMut<'a, CLIStore>, InternalError> {
        Ok(self.store.borrow_mut())
    }
}

impl KVStore for CLIStore {
    fn env(&self) -> &Environment {
        &Environment::Testing
    }

    fn purge(&mut self) -> Result<(), InternalError> {
        self.0.clear();
        Ok(())
    }

    fn set(&mut self, key: &str, value: &str) -> Result<(), InternalError> {
        self.0.insert(key.to_string(), value.to_string());
        Ok(())
    }

    fn get(&mut self, key: &str) -> Result<Option<String>, InternalError> {
        Ok(self.0.get(key).cloned())
    }
}

#[test]
fn skus_e2e_works() {
    let subscriber = FmtSubscriber::builder()
        .with_max_level(Level::TRACE)
        .finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let client = CLIClient {
            store: RefCell::new(CLIStore(HashMap::new())),
        };
        let sdk = skus::sdk::SDK::new(client, Environment::Testing, None, None);
        sdk.initialize().await.unwrap();

        let order = sdk.create_order("trial").await.unwrap();

        sdk.refresh_order(&order.id).await.unwrap();
        // Local cache should return response
        sdk.refresh_order(&order.id).await.unwrap();

        sdk.fetch_order_credentials(&order.id).await.unwrap();

        sdk.present_order_credentials(&order.id, &order.location, "/")
            .await
            .unwrap();
    });
}
