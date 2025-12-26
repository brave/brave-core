use super::{FileLock, SparseIndex};
use crate::{Error, IndexKrate, KrateName};
pub use reqwest::Client as AsyncClient;
pub use reqwest::blocking::Client;
use std::collections::{BTreeMap, BTreeSet};

/// Allows **blocking** access to a remote HTTP sparse registry index
pub struct RemoteSparseIndex {
    /// The local index this remote is wrapping
    pub index: SparseIndex,
    /// The client used to make requests to the remote index
    pub client: Client,
}

impl RemoteSparseIndex {
    /// Creates a new [`Self`] that can access and write local cache entries,
    /// and contact the remote index to retrieve the latest index information
    #[inline]
    pub fn new(index: SparseIndex, client: Client) -> Self {
        Self { index, client }
    }

    /// Gets the latest index metadata for the crate
    ///
    /// Network I/O is _always_ performed when calling this method, however the
    /// response from the remote registry will be empty of contents other than
    /// headers if the local cache entry for the crate is up to date with the
    /// latest in the index
    pub fn krate(
        &self,
        name: KrateName<'_>,
        write_cache_entry: bool,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        let req = self.index.make_remote_request(name, None, lock)?;
        let (
            http::request::Parts {
                method,
                uri,
                version,
                headers,
                ..
            },
            _,
        ) = req.into_parts();

        let mut req = self.client.request(method, uri.to_string());
        req = req.version(version);
        req = req.headers(headers);

        let res = self.client.execute(req.build()?)?;

        let mut builder = http::Response::builder()
            .status(res.status())
            .version(res.version());

        builder
            .headers_mut()
            .unwrap()
            .extend(res.headers().iter().map(|(k, v)| (k.clone(), v.clone())));

        let body = res.bytes()?;
        let res = builder.body(body.to_vec())?;

        self.index
            .parse_remote_response(name, res, write_cache_entry, lock)
    }

    /// Attempts to read the locally cached crate information
    ///
    /// This method does no network I/O unlike [`Self::krate`], but does not
    /// guarantee that the cache information is up to date with the latest in
    /// the remote index
    #[inline]
    pub fn cached_krate(
        &self,
        name: KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        self.index.cached_krate(name, lock)
    }

    /// Helper method for downloading multiple crates in parallel
    ///
    /// Note that in most cases using [`AsyncRemoteSparseIndex::krates_blocking`]
    /// will outperform this method, especially on lower core counts
    #[inline]
    pub fn krates(
        &self,
        mut krates: BTreeSet<String>,
        write_cache_entries: bool,
        lock: &FileLock,
    ) -> BTreeMap<String, Result<Option<IndexKrate>, Error>> {
        let Some(prep_krate) = krates.pop_last() else {
            return Default::default();
        };

        let prep = || {
            let name = prep_krate.as_str().try_into()?;
            self.krate(name, write_cache_entries, lock)
        };

        let prep_krate_res = prep();

        use rayon::prelude::*;
        let mut results: BTreeMap<_, _> = krates
            .into_par_iter()
            .map(|kname| {
                let res = || {
                    let name = kname.as_str().try_into()?;
                    self.krate(name, write_cache_entries, lock)
                };
                let res = res();
                (kname, res)
            })
            .collect();

        results.insert(prep_krate, prep_krate_res);
        results
    }
}

/// Allows **async** access to a remote HTTP sparse registry index
pub struct AsyncRemoteSparseIndex {
    /// The local index this remote is wrapping
    pub index: SparseIndex,
    /// The client used to make requests to the remote index
    pub client: AsyncClient,
}

impl AsyncRemoteSparseIndex {
    /// Creates a new [`Self`] that can access and write local cache entries,
    /// and contact the remote index to retrieve the latest index information
    #[inline]
    pub fn new(index: SparseIndex, client: AsyncClient) -> Self {
        Self { index, client }
    }

    /// Async version of [`RemoteSparseIndex::krate`]
    pub async fn krate_async(
        &self,
        name: KrateName<'_>,
        write_cache_entry: bool,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        let req = self.index.make_remote_request(name, None, lock)?;

        let (
            http::request::Parts {
                method,
                uri,
                version,
                headers,
                ..
            },
            _,
        ) = req.into_parts();

        let mut req = self.client.request(method, uri.to_string());
        req = req.version(version);
        req = req.headers(headers);

        let res = Self::exec_request(&self.client, req.build()?).await?;

        self.index
            .parse_remote_response(name, res, write_cache_entry, lock)
    }

    async fn exec_request(
        client: &AsyncClient,
        req: reqwest::Request,
    ) -> Result<http::Response<Vec<u8>>, Error> {
        // This is unfortunate, but we always make a copy in case we need to retry
        let res = loop {
            let reqc = req.try_clone().unwrap();
            let res = client.execute(reqc).await;

            match res {
                Err(err) if err.is_connect() || err.is_timeout() || err.is_request() => {}
                Err(err) => return Err(err.into()),
                Ok(res) => break res,
            }
        };

        let mut builder = http::Response::builder()
            .status(res.status())
            .version(res.version());

        builder
            .headers_mut()
            .unwrap()
            .extend(res.headers().iter().map(|(k, v)| (k.clone(), v.clone())));

        let body = res.bytes().await?;
        Ok(builder.body(body.to_vec())?)
    }

    /// Attempts to read the locally cached crate information
    ///
    /// This method does no network I/O unlike [`Self::krate_async`], but does not
    /// guarantee that the cache information is up to date with the latest in
    /// the remote index
    #[inline]
    pub fn cached_krate(
        &self,
        name: KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        self.index.cached_krate(name, lock)
    }

    /// Helper method for downloading multiples crates concurrently
    ///
    /// This method will generally perform better than [`RemoteSparseIndex::krates`]
    ///
    /// One notable difference with this method is that you can specify a maximum
    /// duration that each individual krate request can take before it is timed out.
    /// This is because certain [errors](https://github.com/seanmonstar/reqwest/issues/1748)
    /// can occur when making many concurrent requests, which we detect and retry
    /// automatically, but with (by default) no upper bound in number of
    /// retries/time.
    ///
    /// You can also run this entire operation with a single timeout if you wish,
    /// via something like [`tokio::time::timeout`](https://docs.rs/tokio/latest/tokio/time/fn.timeout.html)
    pub async fn krates(
        &self,
        mut krates: BTreeSet<String>,
        write_cache_entries: bool,
        individual_timeout: Option<std::time::Duration>,
        lock: &FileLock,
    ) -> BTreeMap<String, Result<Option<IndexKrate>, Error>> {
        let Some(prep_krate) = krates.pop_last() else {
            return Default::default();
        };

        let create_req = |kname: &str| -> Result<reqwest::Request, Error> {
            let name = kname.try_into()?;
            let req = self.index.make_remote_request(name, None, lock)?;

            let (
                http::request::Parts {
                    method,
                    uri,
                    version,
                    headers,
                    ..
                },
                _,
            ) = req.into_parts();

            let mut req = self.client.request(method, uri.to_string());
            req = req.version(version);
            req = req.headers(headers);

            Ok(req.build()?)
        };

        let mut results = BTreeMap::new();

        {
            let result;
            match create_req(&prep_krate) {
                Ok(req) => match Self::exec_request(&self.client, req).await {
                    Ok(res) => {
                        result = self.index.parse_remote_response(
                            prep_krate.as_str().try_into().unwrap(),
                            res,
                            write_cache_entries,
                            lock,
                        );
                    }
                    Err(err) => result = Err(err),
                },
                Err(err) => result = Err(err),
            }

            results.insert(prep_krate, result);
        }

        let mut tasks = tokio::task::JoinSet::new();

        for kname in krates {
            match create_req(kname.as_str()) {
                Ok(req) => {
                    let client = self.client.clone();
                    tasks.spawn(async move {
                        let res = if let Some(to) = individual_timeout {
                            match tokio::time::timeout(to, Self::exec_request(&client, req)).await {
                                Ok(res) => res,
                                Err(_) => Err(Error::Http(crate::HttpError::Timeout)),
                            }
                        } else {
                            Self::exec_request(&client, req).await
                        };

                        (kname, res)
                    });
                }
                Err(err) => {
                    results.insert(kname, Err(err));
                }
            }
        }

        let (tx, rx) = crossbeam_channel::unbounded();
        while let Some(res) = tasks.join_next().await {
            let Ok(res) = res else {
                continue;
            };
            let _ = tx.send(res);
        }

        drop(tx);

        let results = std::sync::Mutex::new(results);
        rayon::scope(|s| {
            while let Ok((kname, res)) = rx.recv() {
                s.spawn(|_s| {
                    let res = res.and_then(|res| {
                        let name = kname
                            .as_str()
                            .try_into()
                            .expect("this was already validated");
                        self.index
                            .parse_remote_response(name, res, write_cache_entries, lock)
                    });

                    results.lock().unwrap().insert(kname, res);
                });
            }
        });

        results.into_inner().unwrap()
    }

    /// A non-async version of [`Self::krates`]
    ///
    /// Using this method requires that there is an active tokio runtime as
    /// described [here](https://docs.rs/tokio/latest/tokio/runtime/struct.Handle.html#method.current)
    pub fn krates_blocking(
        &self,
        krates: BTreeSet<String>,
        write_cache_entries: bool,
        individual_timeout: Option<std::time::Duration>,
        lock: &FileLock,
    ) -> Result<BTreeMap<String, Result<Option<IndexKrate>, Error>>, tokio::runtime::TryCurrentError>
    {
        let current = tokio::runtime::Handle::try_current()?;
        Ok(current.block_on(async {
            self.krates(krates, write_cache_entries, individual_timeout, lock)
                .await
        }))
    }
}

impl From<reqwest::Error> for Error {
    #[inline]
    fn from(e: reqwest::Error) -> Self {
        Self::Http(crate::HttpError::Reqwest(e))
    }
}

impl From<http::Error> for Error {
    #[inline]
    fn from(e: http::Error) -> Self {
        Self::Http(crate::HttpError::Http(e))
    }
}
