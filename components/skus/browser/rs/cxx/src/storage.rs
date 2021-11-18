use std::cell::RefMut;

use crate::{ffi, NativeClient, NativeClientContext};
use skus::{errors, Environment, KVClient, KVStore};

impl KVClient for NativeClient {
    type Store = NativeClientContext;

    #[allow(clippy::needless_lifetimes)]
    fn get_store<'a>(&'a self) -> Result<RefMut<'a, NativeClientContext>, errors::InternalError> {
        self.ctx
            .try_borrow_mut()
            .or(Err(errors::InternalError::BorrowFailed))
    }
}

impl KVStore for NativeClientContext {
    fn env(&self) -> &Environment {
        &self.environment
    }
    fn purge(&mut self) -> Result<(), errors::InternalError> {
        ffi::shim_purge(self.ctx.pin_mut());
        Ok(())
    }
    fn set(&mut self, key: &str, value: &str) -> Result<(), errors::InternalError> {
        ffi::shim_set(self.ctx.pin_mut(), key, value);
        Ok(())
    }
    fn get(&mut self, key: &str) -> Result<Option<String>, errors::InternalError> {
        let ret = ffi::shim_get(self.ctx.pin_mut(), key);
        Ok(if !ret.is_empty() { Some(ret) } else { None })
    }
}
