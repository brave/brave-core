use async_std::task;
use async_trait::async_trait;
use skus::{errors::InternalError, HTTPClient, KVClient, KVStore};

use isahc::prelude::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::io::Read;
use tracing::{debug, Level};
use tracing_subscriber::FmtSubscriber;

thread_local! {
    pub static STORE: RefCell<HashMap<String,String>> = RefCell::new(HashMap::new());
}

pub struct CLIClient {}

#[async_trait(?Send)]
impl HTTPClient for CLIClient {
    fn get_cookie(name: &str) -> Option<String> {
        debug!("Cookie get: {}", name);
        // FIXME
        None
    }

    fn set_cookie(value: &str) {
        // FIXME
        debug!("Cookie set: {}", value);
    }

    fn schedule_wakeup(delay_ms: u64) {
        // FIXME
        debug!("Schedule wakeup: {}", delay_ms);
    }

    async fn execute(
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

impl KVClient<CLIClient> for CLIClient {
    fn get_store() -> Result<Self, InternalError> {
        Ok(CLIClient {})
    }
}

impl KVStore for CLIClient {
    fn purge(&mut self) -> Result<(), InternalError> {
        STORE.with(|store| {
            store.borrow_mut().clear();
        });
        Ok(())
    }

    fn set(&mut self, key: &str, value: &str) -> Result<(), InternalError> {
        STORE.with(|store| {
            store
                .borrow_mut()
                .insert(key.to_string(), value.to_string());
        });
        Ok(())
    }

    fn get(&mut self, key: &str) -> Result<Option<String>, InternalError> {
        STORE.with(|store| Ok(store.borrow().get(key).cloned()))
    }
}

#[test]
fn skus_e2e_works() {
    let subscriber = FmtSubscriber::builder()
        .with_max_level(Level::TRACE)
        .finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let sdk = skus::sdk::SDK::new::<CLIClient>("testing", None, None);
        sdk.initialize::<CLIClient>().await.unwrap();

        let order = sdk.create_order::<CLIClient>("trial").await.unwrap();

        sdk.refresh_order::<CLIClient>(&order.id).await.unwrap();
        // Local cache should return response
        sdk.refresh_order::<CLIClient>(&order.id).await.unwrap();

        sdk.fetch_order_credentials::<CLIClient>(&order.id)
            .await
            .unwrap();

        sdk.present_order_credentials::<CLIClient>(&order.id, &order.location, "/")
            .await
            .unwrap();
    });
}
