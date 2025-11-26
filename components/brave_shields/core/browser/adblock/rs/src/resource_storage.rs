/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use adblock::resources::{InMemoryResourceStorage, Resource, ResourceImpl, ResourceStorageBackend};
use cxx::CxxString;
use std::sync::Arc;

#[derive(Clone)]
pub struct BraveCoreResourceStorage {
    pub shared_storage: Arc<InMemoryResourceStorage>,
}

impl ResourceStorageBackend for BraveCoreResourceStorage {
    fn get_resource(&self, resource_ident: &str) -> Option<ResourceImpl> {
        self.shared_storage.get_resource(resource_ident)
    }
}

/// Creates a new ResourceStorage from JSON string.
pub fn new_resource_storage(resources_json: &CxxString) -> Box<BraveCoreResourceStorage> {
    let json_str = resources_json.to_str().unwrap_or("[]");
    Box::new(match serde_json::from_str::<Vec<Resource>>(json_str) {
        Ok(resources) => {
            let in_memory_storage = InMemoryResourceStorage::from_resources(resources);
            let shared_storage = Arc::new(in_memory_storage);
            BraveCoreResourceStorage { shared_storage }
        }
        Err(_) => {
            // If parsing fails, create empty storage
            let in_memory_storage = InMemoryResourceStorage::from_resources(vec![]);
            let shared_storage = Arc::new(in_memory_storage);
            BraveCoreResourceStorage { shared_storage }
        }
    })
}

/// Clones a BraveCoreResourceStorage.
/// Clones only the Arc, the storage remains shared.
pub fn clone_resource_storage(storage: &BraveCoreResourceStorage) -> Box<BraveCoreResourceStorage> {
    Box::new(storage.clone())
}

/// Clones and extends a storage with additional resources.
pub fn extend_resource_storage(
    storage: &BraveCoreResourceStorage,
    additional_resources_json: &str,
) -> Box<BraveCoreResourceStorage> {
    if let Ok(additional_resources) =
        serde_json::from_str::<Vec<Resource>>(additional_resources_json)
    {
        if !additional_resources.is_empty() {
            let mut storage = storage.shared_storage.as_ref().clone();

            for resource in additional_resources {
                let _ = storage.add_resource(resource);
            }
            return Box::new(BraveCoreResourceStorage { shared_storage: Arc::new(storage) });
        }
    }
    clone_resource_storage(storage)
}
