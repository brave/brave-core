//! Adds functionality for retrieving crate files from a remote registry

use crate::Error;

/// Wrapper around a [`reqwest::blocking::Client`] to condition it correctly
/// for making requests to a remote registry
#[derive(Clone)]
pub struct Client {
    inner: reqwest::blocking::Client,
}

impl Client {
    /// Creates a client from the specified builder
    pub fn build(builder: reqwest::blocking::ClientBuilder) -> Result<Self, Error> {
        // Crates are _usually_ just stored as content-type: application/gzip
        // without a content-encoding, but _just in case_ we disable gzip so that
        // they aren't automatically decompressed by reqwest, screwing up the
        // checksum computation
        let inner = builder.no_gzip().build()?;
        Ok(Self { inner })
    }
}

impl<'iv> super::ValidKrate<'iv> {
    /// Downloads and validates a .crate from the specified index
    pub fn download(
        client: &Client,
        config: &crate::index::IndexConfig,
        version: &'iv crate::IndexVersion,
    ) -> Result<Self, Error> {
        let url = config.download_url(version.name.as_str().try_into()?, version.version.as_ref());

        let res = client.inner.get(url).send()?.error_for_status()?;
        let body = res.bytes()?;
        Self::validate(body, version)
    }
}
