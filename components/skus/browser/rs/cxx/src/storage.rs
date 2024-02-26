// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use async_trait::async_trait;
use futures::channel::oneshot;
use futures::lock::MutexGuard;
use tracing::debug;

use crate::{ffi, NativeClient, NativeClientInner};
use skus::{errors::InternalError, Environment, KVClient, KVStore};

pub struct StoragePurgeContext {
    tx: oneshot::Sender<Result<(), InternalError>>,
    client: NativeClientInner,
}

pub struct StorageSetContext {
    tx: oneshot::Sender<Result<(), InternalError>>,
    client: NativeClientInner,
}

pub struct StorageGetContext {
    tx: oneshot::Sender<Result<Option<String>, InternalError>>,
    client: NativeClientInner,
}

#[async_trait(?Send)]
impl KVClient for NativeClient {
    type Store = NativeClientInner;
    type StoreRef<'a> = MutexGuard<'a, NativeClientInner>;

    async fn get_store<'a>(&'a self) -> Result<MutexGuard<'a, NativeClientInner>, InternalError> {
        Ok(self.inner.lock().await)
    }
}

#[async_trait(?Send)]
impl KVStore for NativeClientInner {
    fn env(&self) -> &Environment {
        &self.environment
    }
    async fn purge(&mut self) -> Result<(), InternalError> {
        let (tx, rx) = oneshot::channel();
        let context = Box::new(StoragePurgeContext { tx, client: self.clone() });

        ffi::shim_purge(
            self.ctx.try_borrow_mut().map_err(|_| InternalError::BorrowFailed)?.pin_mut(),
            |context, success| {
                let _ =
                    context.tx.send(success.then_some(()).ok_or(
                        InternalError::StorageWriteFailed("prefs write failed".to_string()),
                    ));

                if let Ok(mut executor) = context.client.executor.try_borrow_mut() {
                    executor.try_run_until_stalled()
                }
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
                let _ =
                    context.tx.send(success.then_some(()).ok_or(
                        InternalError::StorageWriteFailed("prefs write failed".to_string()),
                    ));

                if let Ok(mut executor) = context.client.executor.try_borrow_mut() {
                    executor.try_run_until_stalled()
                }
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
                let _ = context.tx.send(
                    success
                        .then_some(if !resp.is_empty() { Some(resp) } else { None })
                        .ok_or(InternalError::StorageReadFailed("prefs read failed".to_string())),
                );

                if let Ok(mut executor) = context.client.executor.try_borrow_mut() {
                    executor.try_run_until_stalled()
                }
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
