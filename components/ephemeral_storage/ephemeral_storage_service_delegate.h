/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_

#include <string>
#include <utility>

#include "content/public/browser/storage_partition_config.h"
#include "url/origin.h"

namespace ephemeral_storage {

// TLD ephemeral area is keyed by the TLD-specific security domain and
// StoragePartitionConfig.
using TLDEphemeralAreaKey =
    std::pair<std::string, content::StoragePartitionConfig>;

// Delegate performs cleanup for all required parts (chrome, content, etc.).
class EphemeralStorageServiceDelegate {
 public:
  virtual ~EphemeralStorageServiceDelegate() = default;

  // Cleanups ephemeral storages (local storage, cookies).
  virtual void CleanupTLDEphemeralArea(const TLDEphemeralAreaKey& key) {}
  // Cleanups non-ephemeral first party storage areas (cache, dom storage).
  virtual void CleanupFirstPartyStorageArea(
      const std::string& registerable_domain) {}
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_DELEGATE_H_
