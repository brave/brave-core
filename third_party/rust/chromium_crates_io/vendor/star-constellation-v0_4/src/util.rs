#[cfg(all(feature = "postcard", feature = "bincode"))]
compile_error!("Features 'postcard' and 'bincode' are mutually exclusive");

#[cfg(not(any(feature = "postcard", feature = "bincode")))]
compile_error!("One of 'postcard' or 'bincode' feature must be enabled");

use serde::{de::DeserializeOwned, Serialize};

use crate::Error;

pub fn serialize<T: Serialize>(value: &T) -> Result<Vec<u8>, Error> {
  #[cfg(all(feature = "bincode", not(feature = "postcard")))]
  {
    return bincode::serialize(value)
      .map_err(|e| Error::Serialization(e.to_string()));
  }
  #[cfg(all(feature = "postcard", not(feature = "bincode")))]
  {
    return postcard::to_stdvec(value)
      .map_err(|e| Error::Serialization(e.to_string()));
  }
  unreachable!()
}

pub fn deserialize<T: DeserializeOwned>(bytes: &[u8]) -> Result<T, Error> {
  #[cfg(all(feature = "bincode", not(feature = "postcard")))]
  {
    return bincode::deserialize(bytes)
      .map_err(|e| Error::Serialization(e.to_string()));
  }
  #[cfg(all(feature = "postcard", not(feature = "bincode")))]
  {
    return postcard::from_bytes(bytes)
      .map_err(|e| Error::Serialization(e.to_string()));
  }
  unreachable!()
}
