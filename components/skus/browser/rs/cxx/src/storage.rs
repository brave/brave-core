use std::cell::RefMut;

use async_trait::async_trait;
use futures::channel::oneshot;
use tracing::debug;

use crate::{ffi, NativeClient, NativeClientContext};
use skus::{errors::InternalError, Environment, KVClient, KVStore};

pub struct StoragePurgeContext {
    tx: oneshot::Sender<Result<(), InternalError>>,
    client: NativeClientContext,
}

pub struct StorageSetContext {
    tx: oneshot::Sender<Result<(), InternalError>>,
    client: NativeClientContext,
}

pub struct StorageGetContext {
    tx: oneshot::Sender<Result<Option<String>, InternalError>>,
    client: NativeClientContext,
}


impl KVClient for NativeClient {
    type Store = NativeClientContext;

    #[allow(clippy::needless_lifetimes)]
    fn get_store<'a>(&'a self) -> Result<RefMut<'a, NativeClientContext>, InternalError> {
        self.ctx.try_borrow_mut().or(Err(InternalError::BorrowFailed))
    }
}

#[async_trait(?Send)]
impl KVStore for NativeClientContext {
    fn env(&self) -> &Environment {
        &self.environment
    }
    async fn purge(&mut self) -> Result<(), InternalError> {
        let (tx, rx) = oneshot::channel();
        let context = Box::new(StoragePurgeContext { tx, client: self.clone() });

        ffi::shim_purge(
            self.ctx.try_borrow_mut().map_err(|_| InternalError::BorrowFailed)?.pin_mut(),
            |context, success| {
                let _ = context.tx.send(success.then_some(()).ok_or(InternalError::StorageWriteFailed("prefs write failed".to_string())));

                context.client.try_run_until_stalled();
            },
            context,
        );
        match rx.await {
            Ok(ret) => ret,
            Err(_) => {
                debug!("purge response channel was cancelled");
                Err(InternalError::FutureCancelled)
            }
        }
    }

    async fn set(&mut self, key: &str, value: &str) -> Result<(), InternalError> {
        let (tx, rx) = oneshot::channel();
        let context = Box::new(StorageSetContext { tx, client: self.clone() });

        ffi::shim_set(
            self.ctx.try_borrow_mut().map_err(|_| InternalError::BorrowFailed)?.pin_mut(),
            key,
            value,
            |context, success| {
                let _ = context.tx.send(success.then_some(()).ok_or(InternalError::StorageWriteFailed("prefs write failed".to_string())));

                context.client.try_run_until_stalled();
            },
            context,
        );
        match rx.await {
            Ok(ret) => ret,
            Err(_) => {
                debug!("purge response channel was cancelled");
                Err(InternalError::FutureCancelled)
            }
        }
    }

    async fn get(&mut self, key: &str) -> Result<Option<String>, InternalError> {

        let (tx, rx) = oneshot::channel();
        let context = Box::new(StorageGetContext { tx, client: self.clone() });

        ffi::shim_get(
            self.ctx.try_borrow_mut().map_err(|_| InternalError::BorrowFailed)?.pin_mut(),
            key,
            |context, resp, success| {
                let _ = context.tx.send(success.then_some(if !resp.is_empty() { Some(resp) } else { None }).ok_or(InternalError::StorageReadFailed("prefs reead failed".to_string())));

                context.client.try_run_until_stalled();
            },
            context,
        );
        match rx.await {
            Ok(ret) => ret,
            Err(_) => {
                debug!("purge response channel was cancelled");
                Err(InternalError::FutureCancelled)
            }
        }
    }
}
