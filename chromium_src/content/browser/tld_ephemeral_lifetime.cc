/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "content/public/browser/tld_ephemeral_lifetime.h"

#include <map>
#include "base/no_destructor.h"
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

TLDEphemeralLifetime::TLDEphemeralLifetime(TLDEphemeralLifetimeKey key,
                                           StoragePartition* storage_partition)
    : key_(key), storage_partition_(storage_partition) {
  DCHECK(active_tld_storage_areas().find(key) ==
         active_tld_storage_areas().end());
  active_tld_storage_areas().emplace(key, weak_factory_.GetWeakPtr());
}

TLDEphemeralLifetime::~TLDEphemeralLifetime() {
  auto filter = network::mojom::CookieDeletionFilter::New();
  filter->ephemeral_storage_domain = key_.second;
  storage_partition_->GetCookieManagerForBrowserProcess()->DeleteCookies(
      std::move(filter), base::NullCallback());

  active_tld_storage_areas().erase(key_);
}

TLDEphemeralLifetime* TLDEphemeralLifetime::Get(BrowserContext* browser_context,
                                                std::string storage_domain) {
  TLDEphemeralLifetimeKey key = std::make_pair(browser_context, storage_domain);
  auto it = active_tld_storage_areas().find(key);
  DCHECK(it == active_tld_storage_areas().end() || it->second.get());
  return it != active_tld_storage_areas().end() ? it->second.get() : nullptr;
}

scoped_refptr<TLDEphemeralLifetime> TLDEphemeralLifetime::GetOrCreate(
    BrowserContext* browser_context,
    StoragePartition* storage_partition,
    std::string storage_domain) {
  if (TLDEphemeralLifetime* existing = Get(browser_context, storage_domain)) {
    return existing;
  }

  TLDEphemeralLifetimeKey key = std::make_pair(browser_context, storage_domain);
  return base::MakeRefCounted<TLDEphemeralLifetime>(key, storage_partition);
}

}  // namespace content
