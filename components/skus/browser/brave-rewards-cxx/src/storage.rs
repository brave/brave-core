use std::ops::DerefMut;

use crate::{ffi, NativeClient, NativeClientContext};
use brave_rewards::{errors, KVClient, KVStore};

pub struct RefMutNativeClientContext(NativeClientContext);

impl KVClient for NativeClient {
    type Store = RefMutNativeClientContext;

    fn get_store(&self) -> Result<RefMutNativeClientContext, errors::InternalError> {
        Ok(RefMutNativeClientContext(self.ctx.to_owned()))
    }
}

impl KVStore for RefMutNativeClientContext {
    fn purge(&mut self) -> Result<(), errors::InternalError> {
        ffi::shim_purge(
            self.0
                 .0
                .try_borrow_mut()
                .or(Err(errors::InternalError::BorrowFailed))?
                .deref_mut()
                .pin_mut(),
        );
        Ok(())
    }
    fn set(&mut self, key: &str, value: &str) -> Result<(), errors::InternalError> {
        ffi::shim_set(
            self.0
                 .0
                .try_borrow_mut()
                .or(Err(errors::InternalError::BorrowFailed))?
                .deref_mut()
                .pin_mut(),
            key,
            value,
        );
        Ok(())
    }
    fn get(&mut self, key: &str) -> Result<Option<String>, errors::InternalError> {
        let ret = ffi::shim_get(
            self.0
                 .0
                .try_borrow_mut()
                .or(Err(errors::InternalError::BorrowFailed))?
                .deref_mut()
                .pin_mut(),
            key,
        );
        Ok(if ret.len() > 0 {
            Some(ret.to_string())
        } else {
            None
        })
    }
}
