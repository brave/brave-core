/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/tld_ephemeral_lifetime.h"

#include <map>

#include "base/no_destructor.h"
#include "base/ranges/algorithm.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"

namespace ephemeral_storage {

namespace {

using TLDEphemeralLifetimeMap =
    std::map<TLDEphemeralLifetimeKey, base::WeakPtr<TLDEphemeralLifetime>>;

// This map allows TLDEphemeralLifetime to manage the lifetime of ephemeral
// storage. We use weak pointers so that we can catch misuse of more easily.
// With weak pointers, these entries will become null if they are destroyed,
// but not removed from the map.
TLDEphemeralLifetimeMap& ActiveTLDStorageAreas() {
  static base::NoDestructor<TLDEphemeralLifetimeMap> active_storage_areas;
  return *active_storage_areas.get();
}

}  // namespace

TLDEphemeralLifetime::TLDEphemeralLifetime(
    const TLDEphemeralLifetimeKey& key,
    const content::StoragePartitionConfig& storage_partition_config)
    : key_(key), storage_partition_config_(storage_partition_config) {
  DCHECK(ActiveTLDStorageAreas().find(key_) == ActiveTLDStorageAreas().end());
  ActiveTLDStorageAreas().emplace(key_, weak_factory_.GetWeakPtr());
  ephemeral_storage_service_ =
      EphemeralStorageServiceFactory::GetForContext(key.first)->GetWeakPtr();
  DCHECK(ephemeral_storage_service_);
  ephemeral_storage_service_->TLDEphemeralLifetimeCreated(
      key.second, storage_partition_config_);
}

TLDEphemeralLifetime::~TLDEphemeralLifetime() {
  if (ephemeral_storage_service_) {
    const bool shields_disabled_on_one_of_hosts = base::ranges::any_of(
        shields_state_on_hosts_, [](const auto& v) { return !v.second; });
    ephemeral_storage_service_->TLDEphemeralLifetimeDestroyed(
        key_.second, storage_partition_config_,
        shields_disabled_on_one_of_hosts);
  }

  ActiveTLDStorageAreas().erase(key_);
}

// static
TLDEphemeralLifetime* TLDEphemeralLifetime::Get(
    content::BrowserContext* browser_context,
    const std::string& storage_domain) {
  const TLDEphemeralLifetimeKey key(browser_context, storage_domain);
  return Get(key);
}

// static
scoped_refptr<TLDEphemeralLifetime> TLDEphemeralLifetime::GetOrCreate(
    content::BrowserContext* browser_context,
    const std::string& storage_domain,
    const content::StoragePartitionConfig& storage_partition_config) {
  const TLDEphemeralLifetimeKey key(browser_context, storage_domain);
  if (scoped_refptr<TLDEphemeralLifetime> existing = Get(key)) {
    CHECK_EQ(existing->storage_partition_config(), storage_partition_config);
    return existing;
  }

  return base::MakeRefCounted<TLDEphemeralLifetime>(key,
                                                    storage_partition_config);
}

// static
TLDEphemeralLifetime* TLDEphemeralLifetime::Get(
    const TLDEphemeralLifetimeKey& key) {
  auto it = ActiveTLDStorageAreas().find(key);
  DCHECK(it == ActiveTLDStorageAreas().end() || it->second.get());
  return it != ActiveTLDStorageAreas().end() ? it->second.get() : nullptr;
}

void TLDEphemeralLifetime::SetShieldsStateOnHost(const std::string& host,
                                                 bool enabled) {
  shields_state_on_hosts_[host] = enabled;
}

}  // namespace ephemeral_storage
