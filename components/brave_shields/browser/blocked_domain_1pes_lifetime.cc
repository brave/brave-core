/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/blocked_domain_1pes_lifetime.h"

#include "base/containers/flat_map.h"
#include "base/memory/ptr_util.h"
#include "base/no_destructor.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "content/public/browser/web_contents.h"
#include "url/url_constants.h"

namespace brave_shields {

namespace {

using BlockedDomain1PESLifetimeMap =
    base::flat_map<BlockedDomain1PESLifetime::Key,
                   base::WeakPtr<BlockedDomain1PESLifetime>>;

BlockedDomain1PESLifetimeMap& blocked_domain_1pes_lifetime_map() {
  static base::NoDestructor<BlockedDomain1PESLifetimeMap> map;
  return *map;
}

}  // namespace

// static
scoped_refptr<BlockedDomain1PESLifetime> BlockedDomain1PESLifetime::GetOrCreate(
    ephemeral_storage::EphemeralStorageService* ephemeral_storage_service,
    const GURL& url) {
  const Key key(ephemeral_storage_service, url::Origin::Create(url).GetURL());
  auto& map = blocked_domain_1pes_lifetime_map();
  auto it = map.find(key);
  DCHECK(it == map.end() || it->second);
  if (it != map.end()) {
    return it->second.get();
  }
  auto instance = base::MakeRefCounted<BlockedDomain1PESLifetime>(key);
  map.emplace(key, instance->AsWeakPtr());
  instance->Start1PESEnableRequest();
  return instance;
}

BlockedDomain1PESLifetime::BlockedDomain1PESLifetime(const Key& key)
    : key_(key) {}

BlockedDomain1PESLifetime::~BlockedDomain1PESLifetime() {
  if (is_1pes_enabled_ == true) {
    key_.first->Set1PESEnabledForUrl(key_.second, false);
  }
  blocked_domain_1pes_lifetime_map().erase(key_);
}

void BlockedDomain1PESLifetime::AddOnReadyCallback(
    base::OnceCallback<void()> on_ready) {
  if (is_1pes_enabled_.has_value()) {
    std::move(on_ready).Run();
  } else {
    on_ready_.push_back(std::move(on_ready));
  }
}

void BlockedDomain1PESLifetime::Start1PESEnableRequest() {
  key_.first->Enable1PESForUrlIfPossible(
      key_.second,
      base::BindOnce(&BlockedDomain1PESLifetime::On1PESEnableRequestComplete,
                     this));
}

void BlockedDomain1PESLifetime::On1PESEnableRequestComplete(bool is_enabled) {
  is_1pes_enabled_ = is_enabled;
  auto on_ready = std::move(on_ready_);
  for (auto& ready_cb : on_ready) {
    std::move(ready_cb).Run();
  }
}

}  // namespace brave_shields
