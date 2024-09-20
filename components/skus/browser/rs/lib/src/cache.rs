// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

use std::collections::HashMap;
use std::iter;
use std::time::Duration;

use chrono::{DateTime, Utc};
use http::Response;

use crate::errors::*;
use crate::http::{clone_resp, delay_from_response};
use crate::sdk::SDK;

struct ExpiringValue<T> {
    expires_at: DateTime<Utc>,
    value: T,
}

impl<T> ExpiringValue<T> {
    fn is_expired(&self) -> bool {
        self.expires_at < Utc::now()
    }
}

pub struct CacheNode<T> {
    children: HashMap<String, CacheNode<T>>,
    data: Option<ExpiringValue<T>>,
}

impl<T> Default for CacheNode<T> {
    fn default() -> Self {
        CacheNode { children: HashMap::new(), data: None }
    }
}

impl<T> CacheNode<T> {
    pub fn insert<'a, I>(&mut self, key: I, value: T, ttl: Duration)
    where
        I: IntoIterator<Item = &'a str>,
    {
        let mut node = self;
        for path in key.into_iter() {
            if !node.children.contains_key(path) {
                node.children.insert(path.to_string(), CacheNode::default());
            }
            node = node
                .children
                .get_mut(path)
                .expect("we already checked and created an entry if it was needed");
        }
        node.data = Some(ExpiringValue {
            expires_at: Utc::now() + chrono::Duration::milliseconds(ttl.as_millis() as i64),
            value,
        });
    }

    pub fn get<'a, I>(&self, key: I) -> Option<&T>
    where
        I: IntoIterator<Item = &'a str>,
    {
        let mut node = self;
        for path in key.into_iter() {
            node = node.children.get(path)?;
        }
        node.data.as_ref().filter(|d| !d.is_expired()).map(|d| &d.value)
    }

    pub fn remove_path<'a, I>(&mut self, key: I)
    where
        I: IntoIterator<Item = &'a str>,
    {
        if let Some(node) = key.into_iter().try_fold(self, |node, path| {
            if node.data.is_none() { node.children.get_mut(path) } else { Some(node) }
        }) {
            if node.data.is_some() {
                node.data = None;
                node.children.clear();
            }
        }
    }
}

#[allow(clippy::single_char_pattern)]
fn cache_path_from_method_and_uri<'a>(
    method: &'a http::Method,
    uri: &'a http::Uri,
) -> iter::Chain<iter::Take<iter::Repeat<&'a str>>, std::str::Split<'a, &'a str>> {
    iter::repeat(method.as_str()).take(1).chain(uri.path().split("/"))
}

impl<U> SDK<U> {
    pub fn lookup_cached_response(
        &self,
        method: &http::Method,
        uri: &http::Uri,
    ) -> Result<Option<Response<Vec<u8>>>, InternalError> {
        let cache_key = cache_path_from_method_and_uri(method, uri);

        Ok(self
            .cache
            .try_borrow()
            .map_err(|_| InternalError::RetryLater(None))?
            .get(cache_key)
            .map(clone_resp))
    }

    pub fn cache_request(
        &self,
        method: &http::Method,
        uri: &http::Uri,
        resp: &Response<Vec<u8>>,
    ) -> Result<(), InternalError> {
        let cache_key = cache_path_from_method_and_uri(method, uri);
        // Used for invalidation during mutating methods
        let get_cache_key = cache_path_from_method_and_uri(&http::Method::GET, uri);

        match resp.status() {
            // Cache 429 responses so we don't exceed advised retry-after
            http::StatusCode::TOO_MANY_REQUESTS => {
                if let Some(delay) = delay_from_response(resp) {
                    self.cache
                        .try_borrow_mut()
                        .map_err(|_| InternalError::RetryLater(None))?
                        .insert(cache_key, clone_resp(resp), delay);
                }
            }
            // Cache 200 OK on GET requests for 1 second.
            // This is a safety net to prevent unncessary requests to the server
            // within a short window
            http::StatusCode::OK if *method == http::Method::GET => self
                .cache
                .try_borrow_mut()
                .map_err(|_| InternalError::RetryLater(None))?
                .insert(cache_key, clone_resp(resp), Duration::from_secs(1)),
            _ => match *method {
                // Mutating methods invalidate cached GET requests by including any parent cached
                // values on the path to the cache root. For example if there is a cached response
                // for `GET /v1/orders/XXX` then a `POST /v1/orders/XXX/credentials`
                // will invalidate it and any other cached subresources with
                // `/v1/orders/XXX` as a prefix
                http::Method::DELETE | http::Method::POST | http::Method::PUT => self
                    .cache
                    .try_borrow_mut()
                    .map_err(|_| InternalError::RetryLater(None))?
                    .remove_path(get_cache_key),
                _ => (),
            },
        };
        Ok(())
    }
}
