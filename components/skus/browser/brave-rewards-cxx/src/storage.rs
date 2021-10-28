use std::cell::RefMut;

use crate::{ffi, NativeClient, NativeClientContext};
use brave_rewards::{errors, KVClient, KVStore};

impl KVClient for NativeClient {
    type Store = NativeClientContext;

    fn get_store<'a>(&'a self) -> Result<RefMut<'a, NativeClientContext>, errors::InternalError> {
        Ok(self
            .ctx
            .try_borrow_mut()
            .or(Err(errors::InternalError::BorrowFailed))?)
    }
}

impl KVStore for NativeClientContext {
    fn purge(&mut self) -> Result<(), errors::InternalError> {
        ffi::shim_purge(self.0.pin_mut());
        Ok(())
    }
    fn set(&mut self, key: &str, value: &str) -> Result<(), errors::InternalError> {
        ffi::shim_set(self.0.pin_mut(), key, value);
        Ok(())
    }
    fn get(&mut self, key: &str) -> Result<Option<String>, errors::InternalError> {
        let ret = ffi::shim_get(self.0.pin_mut(), key);
        Ok(if ret.len() > 0 {
            Some(ret.to_string())
        } else {
            None
        })
    }
}
