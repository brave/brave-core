/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_

#include <string>
#include <utility>

#include "content/public/browser/storage_partition_config.h"
#include "url/origin.h"

namespace ephemeral_storage {

// Storage isolation modes
enum class StorageIsolationMode {
  kNone,           // No isolation (default)
  kEphemeral,      // Current ephemeral storage
  kPuppeteer       // New: Puppeteer-specific isolation
};

// Extended key type supporting puppeteer mode isolation
struct ExtendedStorageKey {
  ExtendedStorageKey();
  ExtendedStorageKey(const std::string& domain,
                     const content::StoragePartitionConfig& config,
                     StorageIsolationMode mode = StorageIsolationMode::kNone,
                     const url::Origin& origin = url::Origin());
  ExtendedStorageKey(const ExtendedStorageKey& other);
  ExtendedStorageKey(ExtendedStorageKey&& other) noexcept;
  ExtendedStorageKey& operator=(const ExtendedStorageKey& other);
  ExtendedStorageKey& operator=(ExtendedStorageKey&& other) noexcept;
  ~ExtendedStorageKey();

  std::string tld_domain;
  content::StoragePartitionConfig storage_config;
  StorageIsolationMode isolation_mode = StorageIsolationMode::kNone;
  url::Origin requesting_origin;  // For puppeteer mode

  bool operator<(const ExtendedStorageKey& other) const {
    if (tld_domain != other.tld_domain)
      return tld_domain < other.tld_domain;
    if (storage_config != other.storage_config)
      return storage_config < other.storage_config;
    if (isolation_mode != other.isolation_mode)
      return isolation_mode < other.isolation_mode;
    return requesting_origin < other.requesting_origin;
  }

  bool operator==(const ExtendedStorageKey& other) const {
    return tld_domain == other.tld_domain &&
           storage_config == other.storage_config &&
           isolation_mode == other.isolation_mode &&
           requesting_origin == other.requesting_origin;
  }
};

// TLD ephemeral area is keyed by the TLD-specific security domain and
// StoragePartitionConfig.
using TLDEphemeralAreaKey =
    std::pair<std::string, content::StoragePartitionConfig>;

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_TYPES_H_
