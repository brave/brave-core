use std::cell::RefCell;
use std::{thread, time};
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

#[derive(Debug)]
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
    let subscriber = FmtSubscriber::builder().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let client = CLIClient { store: RefCell::new(CLIStore(HashMap::new())) };
        let sdk = skus::sdk::SDK::new(client, Environment::Testing, None, None);
        sdk.initialize().await;

        let order = sdk.create_order("trial").await.unwrap();

        sdk.refresh_order(&order.id).await.unwrap();
        // Local cache should return response
        sdk.refresh_order(&order.id).await.unwrap();

        sdk.fetch_order_credentials(&order.id).await.unwrap();

        sdk.present_order_credentials(&order.id, &order.location, "/").await.unwrap();
    });
}

#[test]
fn skus_tlv2_e2e_works() {
    let subscriber = FmtSubscriber::builder().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let client = CLIClient { store: RefCell::new(CLIStore(HashMap::new())) };
        let sdk = skus::sdk::SDK::new(client, Environment::Testing, None, None);
        sdk.initialize().await;

        let order = sdk.create_order("tlv2_e2e").await.unwrap();

        sdk.refresh_order(&order.id).await.unwrap();
        // Local cache should return response
        sdk.refresh_order(&order.id).await.unwrap();

        // initialize
        sdk.submit_order_credentials_to_sign(&order.id);

        // go ahead and see if we attempt to re-initialize, hope not
        sdk.fetch_order_credentials(&order.id).await.unwrap();

        for _ in 1..=3 {
            sdk.present_order_credentials(&order.id, &order.location, "/").await.unwrap();
        }
        // should all be spent by this point, only 5 per day per that sku
        match sdk.present_order_credentials(&order.id, &order.location, "/").await {
            Err(sku_err) => {
                println!("{}", sku_err);
            }
            _ => {
                assert!(false); // should have been out of credentials
            }
        }
        // get the existing creds we created, and delete 29 of them in the future so
        // we can test out the reloading
        sdk.remove_last_n_creds(&order.id, 29);
        // reload to get updated creds
        sdk.refresh_order_credentials(&order.id).await.unwrap();

        // fetch again and make sure we have still presented all creds
        sdk.fetch_order_credentials(&order.id).await.unwrap();
        match sdk.present_order_credentials(&order.id, &order.location, "/").await {
            Err(sku_err) => {
                println!("{}", sku_err);
            }
            _ => {
                assert!(false); // should have been out of credentials
            }
        }
    });
}

#[test]
fn has_credentials_works() {
    let subscriber = FmtSubscriber::builder().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let client = CLIClient { store: RefCell::new(CLIStore(HashMap::new())) };
        let sdk = skus::sdk::SDK::new(client, Environment::Testing, None, None);
        sdk.initialize().await;

        let order = sdk.create_order("tlv2_e2e").await.unwrap();

        // calling refresh_order_credentials with no credentials loaded
        sdk.refresh_order_credentials(&order.id).await.unwrap();
    });
}

#[test]
fn skus_5m_tlv2_e2e_works() {
    let subscriber = FmtSubscriber::builder().with_max_level(Level::TRACE).finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    task::block_on(async {
        let client = CLIClient { store: RefCell::new(CLIStore(HashMap::new())) };
        let sdk = skus::sdk::SDK::new(client, Environment::Staging, None, None);
        sdk.initialize().await;

        let order = sdk.create_order("tlv2_e2e_5m").await.unwrap();

        sdk.refresh_order(&order.id).await.unwrap();
        // Local cache should return response
        sdk.refresh_order(&order.id).await.unwrap();

        // initialize
        sdk.submit_order_credentials_to_sign(&order.id);

        // go ahead and see if we attempt to re-initialize, hope not
        sdk.fetch_order_credentials(&order.id).await.unwrap();

		let four_min = time::Duration::from_millis(4*60000);

		for _ in 1..=30 {
        	sdk.present_order_credentials(&order.id, &order.location, "/").await.unwrap();
			let now = time::Instant::now();
			thread::sleep(four_min);
		}
    });
}
