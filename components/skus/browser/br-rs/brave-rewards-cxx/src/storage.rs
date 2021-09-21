use crate::{NativeClient, LOCAL_POOL};
use brave_rewards::{errors, KVClient, KVStore};

impl KVClient<NativeClient> for NativeClient {
    fn get_store() -> Result<Self, errors::InternalError> {
        // FIXME
        return Ok(NativeClient());
    }
}

impl KVStore for NativeClient {
    fn purge(&mut self) -> Result<(), errors::InternalError> {
        Ok(())
        //unimplemented!();
    }
    fn set(&mut self, _key: &str, _value: &str) -> Result<(), errors::InternalError> {
        Ok(())
        //unimplemented!();
    }
    fn get(&mut self, _key: &str) -> Result<Option<String>, errors::InternalError> {
        Ok(None)
        //unimplemented!();
    }
}
