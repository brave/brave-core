mod kv;

use async_trait::async_trait;
use challenge_bypass_ristretto::voprf::*;

pub use kv::{KVClient, KVStore};

use crate::errors::InternalError;
use crate::models::*;

#[async_trait(?Send)]
pub trait StorageClient {
    async fn clear(&self) -> Result<(), InternalError>;
    async fn insert_wallet(&self, wallet: &Wallet) -> Result<(), InternalError>;
    async fn replace_promotions(&self, promotions: &[Promotion]) -> Result<(), InternalError>;
    async fn get_orders(&self) -> Result<Option<Vec<Order>>, InternalError>;
    async fn get_order(&self, order_id: &str) -> Result<Option<Order>, InternalError>;
    async fn upsert_order(&self, order: &Order) -> Result<(), InternalError>;
    async fn delete_item_creds(&self, item_id: &str) -> Result<(), InternalError>;
    async fn get_single_use_item_creds(
        &self,
        item_id: &str,
    ) -> Result<Option<SingleUseCredentials>, InternalError>;
    async fn init_single_use_item_creds(
        &self,
        item_id: &str,
        creds: Vec<Token>,
    ) -> Result<(), InternalError>;
    async fn complete_single_use_item_creds(
        &self,
        item_id: &str,
        issuer_id: &str,
        unblinded_creds: Vec<UnblindedToken>,
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
