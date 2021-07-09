/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/tld_ephemeral_lifetime.h"

#include <map>

#include "base/no_destructor.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"

namespace content {

namespace {

using TLDEphemeralLifetimeMap =
    std::map<TLDEphemeralLifetimeKey, base::WeakPtr<TLDEphemeralLifetime>>;

// This map allows TLDEphemeralLifetime to manage the lifetime of ephemeral
// storage. We use weak pointers so that we can catch misuse of more easily.
// With weak pointers, these entries will become null if they are destroyed,
// but not removed from the map.
TLDEphemeralLifetimeMap& active_tld_storage_areas() {
  static base::NoDestructor<TLDEphemeralLifetimeMap> active_storage_areas;
  return *active_storage_areas.get();
}

}  // namespace

TLDEphemeralLifetime::TLDEphemeralLifetime(
    const TLDEphemeralLifetimeKey& key,
    StoragePartition* storage_partition,
    std::unique_ptr<EphemeralStorageOriginsSource>
        ephemeral_storage_origins_source)
    : key_(key),
      storage_partition_(storage_partition),
      ephemeral_storage_origins_source_(
          std::move(ephemeral_storage_origins_source)) {
  DCHECK(active_tld_storage_areas().find(key_) ==
         active_tld_storage_areas().end());
  DCHECK(storage_partition_);
  DCHECK(ephemeral_storage_origins_source_);
  active_tld_storage_areas().emplace(key_, weak_factory_.GetWeakPtr());
}

TLDEphemeralLifetime::~TLDEphemeralLifetime() {
  auto filter = network::mojom::CookieDeletionFilter::New();
  filter->ephemeral_storage_domain = key_.second;
  storage_partition_->GetCookieManagerForBrowserProcess()->DeleteCookies(
      std::move(filter), base::NullCallback());
  for (const auto& opaque_origin :
       ephemeral_storage_origins_source_->TakeEphemeralStorageOpaqueOrigins(
           key_.second)) {
    storage_partition_->GetDOMStorageContext()->DeleteLocalStorage(
        opaque_origin, base::DoNothing());
  }

  if (!on_destroy_callbacks_.empty()) {
    auto on_destroy_callbacks = std::move(on_destroy_callbacks_);
    for (auto& callback : on_destroy_callbacks) {
      std::move(callback).Run(key_.second);
    }
  }

  active_tld_storage_areas().erase(key_);
}

// static
TLDEphemeralLifetime* TLDEphemeralLifetime::Get(
    BrowserContext* browser_context,
    const std::string& storage_domain) {
  const TLDEphemeralLifetimeKey key(browser_context, storage_domain);
  return Get(key);
}

// static
scoped_refptr<TLDEphemeralLifetime> TLDEphemeralLifetime::GetOrCreate(
    BrowserContext* browser_context,
    StoragePartition* storage_partition,
    const std::string& storage_domain,
    std::unique_ptr<EphemeralStorageOriginsSource>
        ephemeral_storage_origins_source) {
  const TLDEphemeralLifetimeKey key(browser_context, storage_domain);
  if (scoped_refptr<TLDEphemeralLifetime> existing = Get(key)) {
    return existing;
  }

  return base::MakeRefCounted<TLDEphemeralLifetime>(
      key, storage_partition, std::move(ephemeral_storage_origins_source));
}

// static
TLDEphemeralLifetime* TLDEphemeralLifetime::Get(
    const TLDEphemeralLifetimeKey& key) {
  auto it = active_tld_storage_areas().find(key);
  DCHECK(it == active_tld_storage_areas().end() || it->second.get());
  return it != active_tld_storage_areas().end() ? it->second.get() : nullptr;
}

void TLDEphemeralLifetime::RegisterOnDestroyCallback(
    OnDestroyCallback callback) {
  on_destroy_callbacks_.push_back(std::move(callback));
}

}  // namespace content
