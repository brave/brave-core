/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/first_party_storage_lifetime.h"

#include <map>

#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"

namespace ephemeral_storage {

namespace {

using FirstPartyStorageLifetimeMap =
    std::map<FirstPartyStorageLifetimeKey,
             base::WeakPtr<FirstPartyStorageLifetime>>;

FirstPartyStorageLifetimeMap& ActiveStorageAreas() {
  static base::NoDestructor<FirstPartyStorageLifetimeMap> active_storage_areas;
  return *active_storage_areas.get();
}

}  // namespace

FirstPartyStorageLifetime::FirstPartyStorageLifetime(
    const FirstPartyStorageLifetimeKey& key)
    : key_(key) {
  DCHECK(!base::Contains(ActiveStorageAreas(), key_));
  ActiveStorageAreas().emplace(key_, weak_factory_.GetWeakPtr());
  ephemeral_storage_service_ =
      EphemeralStorageServiceFactory::GetForContext(key.first)->GetWeakPtr();
  DCHECK(ephemeral_storage_service_);
  ephemeral_storage_service_->FirstPartyStorageAreaInUse(key_.second);
}

FirstPartyStorageLifetime::~FirstPartyStorageLifetime() {
  if (ephemeral_storage_service_) {
    ephemeral_storage_service_->FirstPartyStorageAreaNotInUse(key_.second);
  }
  ActiveStorageAreas().erase(key_);
}

// static
FirstPartyStorageLifetime* FirstPartyStorageLifetime::Get(
    content::BrowserContext* browser_context,
    const url::Origin& origin) {
  const FirstPartyStorageLifetimeKey key(browser_context, origin);
  return Get(key);
}

// static
scoped_refptr<FirstPartyStorageLifetime> FirstPartyStorageLifetime::GetOrCreate(
    content::BrowserContext* browser_context,
    const url::Origin& origin) {
  const FirstPartyStorageLifetimeKey key(browser_context, origin);
  if (scoped_refptr<FirstPartyStorageLifetime> existing = Get(key)) {
    return existing;
  }

  return base::MakeRefCounted<FirstPartyStorageLifetime>(key);
}

// static
FirstPartyStorageLifetime* FirstPartyStorageLifetime::Get(
    const FirstPartyStorageLifetimeKey& key) {
  auto& areas = ActiveStorageAreas();
  auto it = areas.find(key);
  DCHECK(it == areas.end() || it->second.get());
  return it != areas.end() ? it->second.get() : nullptr;
}

}  // namespace ephemeral_storage
