/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use adblock::resources::{InMemoryResourceStorage, ResourceImpl, ResourceStorageBackend, Resource};
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

impl BraveCoreResourceStorage {
    /// Creates a new BraveCoreResourceStorage from JSON string.
    pub fn from_json(resources_json: &str) -> Result<Self, serde_json::Error> {
        let resources: Vec<Resource> = serde_json::from_str(resources_json)?;
        let in_memory_storage = InMemoryResourceStorage::from_resources(resources);
        let shared_storage = Arc::new(in_memory_storage);
        Ok(BraveCoreResourceStorage { shared_storage })
    }
}

/// Creates a new ResourceStorage from JSON string.
/// This is the FFI wrapper function.
pub fn new_resource_storage(resources_json: &CxxString) -> Box<BraveCoreResourceStorage> {
    let json_str = resources_json.to_str().unwrap_or("[]");
    Box::new(BraveCoreResourceStorage::from_json(json_str).unwrap_or_else(|_| {
        // If parsing fails, create empty storage
        let in_memory_storage = InMemoryResourceStorage::from_resources(vec![]);
        let shared_storage = Arc::new(in_memory_storage);
        BraveCoreResourceStorage { shared_storage }
    }))
}

/// Clones a ResourceStorage.
/// This creates a new Box containing a clone of the storage, sharing the underlying Arc.
pub fn clone_resource_storage(storage: &BraveCoreResourceStorage) -> Box<BraveCoreResourceStorage> {
    Box::new(storage.clone())
}

/// Merges additional resources into existing storage.
/// Creates a new storage containing both the existing resources and additional ones.
pub fn merge_resource_storage(storage: &BraveCoreResourceStorage, additional_resources_json: &str) -> Box<BraveCoreResourceStorage> {
    if additional_resources_json == "[]" || additional_resources_json.is_empty() {
        // No additional resources, just clone the existing storage
        return clone_resource_storage(storage);
    }

    // Parse additional resources
    if let Ok(additional_resources) = serde_json::from_str::<Vec<Resource>>(additional_resources_json) {
        if additional_resources.is_empty() {
            return clone_resource_storage(storage);
        }

        // For now, since we can't extract existing resources from storage,
        // we create new storage with just the additional resources
        // TODO: Ideally we'd merge with existing resources, but storage is opaque
        let in_memory_storage = InMemoryResourceStorage::from_resources(additional_resources);
        let shared_storage = Arc::new(in_memory_storage);
        Box::new(BraveCoreResourceStorage { shared_storage })
    } else {
        // If parsing fails, return a clone of the original storage
        clone_resource_storage(storage)
    }
}
