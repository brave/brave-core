use std::cell::RefMut;
use std::collections::HashMap;
use std::fmt::Debug;

use async_trait::async_trait;
use challenge_bypass_ristretto::voprf::*;

use crate::errors::InternalError;
use crate::models::*;
use crate::StorageClient;
use serde::{Deserialize, Serialize};
use tracing::{event, instrument, Level};

#[derive(Debug, Serialize, Deserialize)]
#[serde(rename_all = "kebab-case")]
#[serde(tag = "type")]
enum Credentials {
    SingleUse(SingleUseCredentials),
    TimeLimited(TimeLimitedCredentials),
}

#[derive(Debug, Serialize, Deserialize)]
struct CredentialsState {
    pub items: HashMap<String, Credentials>,
}

#[derive(Debug, Serialize, Deserialize)]
struct KVState {
    pub wallet: Option<Wallet>,
    pub promotions: Option<Vec<Promotion>>,
    pub orders: Option<HashMap<String, Order>>,
    pub credentials: Option<CredentialsState>,
}

trait KVStoreHelpers<T: KVStore> {
    fn get_state(&mut self) -> Result<KVState, InternalError>;
    fn set_state(&mut self, state: &KVState) -> Result<(), InternalError>;
}

pub trait KVClient {
    type Store;

    #[allow(clippy::needless_lifetimes)]
    fn get_store<'a>(&'a self) -> Result<RefMut<'a, Self::Store>, InternalError>
    where
        Self::Store: KVStore;
}

pub trait KVStore: Sized {
    fn env(&self) -> &Environment;
    fn purge(&mut self) -> Result<(), InternalError>;
    fn set(&mut self, key: &str, value: &str) -> Result<(), InternalError>;
    fn get(&mut self, key: &str) -> Result<Option<String>, InternalError>;
}

fn key_from_environment(env: &Environment) -> String {
    format!("skus:{}", env)
}

impl<C> KVStoreHelpers<C> for C
where
    C: KVStore,
{
    fn get_state(&mut self) -> Result<KVState, InternalError> {
        let key = key_from_environment(self.env());
        if let Ok(Some(state)) = self.get("rewards:local") {
            // Perform a one time migration, clearing any old values
            self.purge()?;
            // and setting a new key with the prior value
            self.set(&key, &state)?;
        }
        let state = self.get(&key)?.unwrap_or_else(|| "{}".to_string());
        Ok(serde_json::from_str(&state)?)
    }

    fn set_state(&mut self, state: &KVState) -> Result<(), InternalError> {
        let key = key_from_environment(self.env());
        event!(Level::DEBUG, "set state");
        event!(Level::TRACE, state = %format!("{:#?}", state), "set state",);
        self.set(&key, &serde_json::to_string(state)?)
    }
}

#[async_trait(?Send)]
impl<C> StorageClient for C
where
    C: KVClient + Debug,
    <C as KVClient>::Store: KVStore,
{
    #[instrument]
    async fn clear(&self) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        store.purge()
    }

    #[instrument]
    async fn insert_wallet(&self, wallet: &Wallet) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if state.wallet.is_none() {
            state.wallet = Some(wallet.clone());
        }

        store.set_state(&state)
    }

    #[instrument]
    async fn replace_promotions(&self, promotions: &[Promotion]) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        state.promotions = Some(promotions.to_vec());

        store.set_state(&state)
    }

    #[instrument]
    async fn get_orders(&self) -> Result<Option<Vec<Order>>, InternalError> {
        let mut store = self.get_store()?;
        let state: KVState = store.get_state()?;
        let orders = state
            .orders
            .map(|os| os.into_iter().map(|(_, order)| order).collect());
        event!(Level::DEBUG, orders = ?orders, "got orders");
        Ok(orders)
    }

    #[instrument]
    async fn get_order(&self, order_id: &str) -> Result<Option<Order>, InternalError> {
        let mut store = self.get_store()?;
        let state: KVState = store.get_state()?;
        let order = state.orders.and_then(|mut orders| orders.remove(order_id));
        event!(Level::DEBUG, order = ?order, "got order");
        Ok(order)
    }

    #[instrument]
    async fn upsert_order(&self, order: &Order) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if state.orders.is_none() {
            state.orders = Some(HashMap::new());
        }

        if let Some(orders) = state.orders.as_mut() {
            orders.insert(order.id.clone(), order.clone());
        }

        store.set_state(&state)
    }

    #[instrument]
    async fn delete_item_creds(&self, item_id: &str) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if let Some(mut credentials) = state.credentials {
            credentials.items.remove(item_id);
            state.credentials = Some(credentials);
        }

        store.set_state(&state)
    }

    #[instrument]
    async fn get_single_use_item_creds(
        &self,
        item_id: &str,
    ) -> Result<Option<SingleUseCredentials>, InternalError> {
        let mut store = self.get_store()?;
        let state: KVState = store.get_state()?;
        let credentials = state.credentials.and_then(|mut credentials| {
            if let Some(Credentials::SingleUse(credentials)) = credentials.items.remove(item_id) {
                Some(credentials)
            } else {
                None
            }
        });
        event!(Level::DEBUG, credentials = ?credentials, "got credentials");
        return Ok(credentials);
    }

    #[instrument]
    async fn init_single_use_item_creds(
        &self,
        item_id: &str,
        creds: Vec<Token>,
    ) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if state.credentials.is_none() {
            state.credentials = Some(CredentialsState {
                items: HashMap::new(),
            });
        }

        if let Some(credentials) = state.credentials.as_mut() {
            let creds = Credentials::SingleUse(SingleUseCredentials {
                item_id: item_id.to_string(),
                creds,
                unblinded_creds: None,
                issuer_id: None,
            });
            if credentials
                .items
                .insert(item_id.to_string(), creds)
                .is_some()
            {
                return Err(InternalError::StorageWriteFailed(
                    "Item credentials were already initialized".to_string(),
                ));
            }
        }

        store.set_state(&state)
    }

    #[instrument]
    async fn complete_single_use_item_creds(
        &self,
        item_id: &str,
        issuer_id: &str,
        unblinded_creds: Vec<UnblindedToken>,
    ) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if let Some(credentials) = state.credentials.as_mut() {
            if let Some(item_credentials) = credentials.items.get_mut(item_id) {
                match item_credentials {
                    Credentials::SingleUse(item_credentials) => {
                        item_credentials.unblinded_creds = Some(
                            unblinded_creds
                                .into_iter()
                                .map(|unblinded_cred| SingleUseCredential {
                                    unblinded_cred,
                                    spent: false,
                                })
                                .collect(),
                        );
                        item_credentials.issuer_id = Some(issuer_id.to_string());
                    }
                    _ => {
                        return Err(InternalError::StorageWriteFailed(
                            "Item is not single use".to_string(),
                        ))
                    }
                }
                return store.set_state(&state);
            }
        }
        Err(InternalError::StorageWriteFailed(
            "Item credentials were not initiated".to_string(),
        ))
    }

    #[instrument]
    async fn spend_single_use_item_cred(
        &self,
        item_id: &str,
        index: usize,
    ) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if let Some(credentials) = state.credentials.as_mut() {
            if let Some(item_credentials) = credentials.items.get_mut(item_id) {
                match item_credentials {
                    Credentials::SingleUse(item_credentials) => {
                        if let Some(unblinded_creds) = item_credentials.unblinded_creds.as_mut() {
                            unblinded_creds[index].spent = true;

                            return store.set_state(&state);
                        }
                    }
                    _ => {
                        return Err(InternalError::StorageWriteFailed(
                            "Item is not single use".to_string(),
                        ))
                    }
                }
            }
        }
        Err(InternalError::StorageWriteFailed(
            "Item credentials were not completed".to_string(),
        ))
    }

    #[instrument]
    async fn get_time_limited_creds(
        &self,
        item_id: &str,
    ) -> Result<Option<TimeLimitedCredentials>, InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if state.credentials.is_none() {
            state.credentials = Some(CredentialsState {
                items: HashMap::new(),
            });
        }

        if let Some(credentials) = state.credentials.as_mut() {
            if let Some(item_credentials) = credentials.items.remove(item_id) {
                match item_credentials {
                    Credentials::TimeLimited(credentials) => {
                        event!(Level::DEBUG, credentials = ?credentials, "got credentials");
                        return Ok(Some(credentials));
                    }
                    _ => {
                        return Err(InternalError::StorageReadFailed(
                            "Item is not time limited".to_string(),
                        ))
                    }
                }
            }
        }
        let credentials = None;
        event!(Level::DEBUG, credentials = ?credentials, "got credentials");
        return Ok(credentials);
    }

    #[instrument]
    async fn store_time_limited_creds(
        &self,
        item_id: &str,
        creds: Vec<TimeLimitedCredential>,
    ) -> Result<(), InternalError> {
        let mut store = self.get_store()?;
        let mut state: KVState = store.get_state()?;

        if state.credentials.is_none() {
            state.credentials = Some(CredentialsState {
                items: HashMap::new(),
            });
        }

        if let Some(credentials) = state.credentials.as_mut() {
            let creds = Credentials::TimeLimited(TimeLimitedCredentials {
                item_id: item_id.to_string(),
                creds,
            });

            // assume that the credentials provided supercede the ones
            // we may already have, so always overwrite the credentials
            credentials.items.insert(item_id.to_string(), creds);
        }

        store.set_state(&state)
    }
}
