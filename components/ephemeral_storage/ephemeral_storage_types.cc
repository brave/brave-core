/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_types.h"

namespace ephemeral_storage {

ExtendedStorageKey::ExtendedStorageKey() = default;

ExtendedStorageKey::ExtendedStorageKey(
    const std::string& domain,
    const content::StoragePartitionConfig& config,
    StorageIsolationMode mode,
    const url::Origin& origin)
    : tld_domain(domain),
      storage_config(config),
      isolation_mode(mode),
      requesting_origin(origin) {}

ExtendedStorageKey::ExtendedStorageKey(const ExtendedStorageKey& other) = default;

ExtendedStorageKey::ExtendedStorageKey(ExtendedStorageKey&& other) noexcept = default;

ExtendedStorageKey& ExtendedStorageKey::operator=(const ExtendedStorageKey& other) = default;

ExtendedStorageKey& ExtendedStorageKey::operator=(ExtendedStorageKey&& other) noexcept = default;

ExtendedStorageKey::~ExtendedStorageKey() = default;

}  // namespace ephemeral_storage