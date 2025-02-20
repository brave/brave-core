/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/permission_origin_lifetime_monitor_impl.h"

#include <utility>

#include "base/containers/contains.h"
#include "brave/browser/ephemeral_storage/ephemeral_storage_service_factory.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

namespace permissions {

PermissionOriginLifetimeMonitorImpl::PermissionOriginLifetimeMonitorImpl(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage));
}

PermissionOriginLifetimeMonitorImpl::~PermissionOriginLifetimeMonitorImpl() =
    default;

void PermissionOriginLifetimeMonitorImpl::
    SetOnPermissionOriginDestroyedCallback(
        base::RepeatingCallback<void(const std::string&)> callback) {
  permission_destroyed_callback_ = std::move(callback);
}

std::string
PermissionOriginLifetimeMonitorImpl::SubscribeToPermissionOriginDestruction(
    const GURL& requesting_origin) {
  DCHECK(permission_destroyed_callback_);
  if (!ephemeral_storage_observation_.IsObserving()) {
    auto* ephemeral_storage_service =
        EphemeralStorageServiceFactory::GetForContext(browser_context_);
    CHECK(ephemeral_storage_service);
    ephemeral_storage_observation_.Observe(ephemeral_storage_service);
  }

  url::Origin sub_request_origin;
  bool is_sub_request_origin = false;
  for (auto type : {RequestType::kBraveEthereum, RequestType::kBraveSolana}) {
    if (brave_wallet::ParseRequestingOriginFromSubRequest(
            type, url::Origin::Create(requesting_origin), &sub_request_origin,
            nullptr)) {
      is_sub_request_origin = true;
      break;
    }
  }
  std::string storage_domain = net::URLToEphemeralStorageDomain(
      is_sub_request_origin ? sub_request_origin.GetURL() : requesting_origin);
  if (!base::Contains(active_subscriptions_, storage_domain)) {
    active_subscriptions_.insert(storage_domain);
  }
  return storage_domain;
}

void PermissionOriginLifetimeMonitorImpl::OnCleanupTLDEphemeralArea(
    const ephemeral_storage::TLDEphemeralAreaKey& key) {
  const auto& storage_domain = key.first;
  auto subscription_it = active_subscriptions_.find(storage_domain);
  if (subscription_it == active_subscriptions_.end()) {
    return;
  }
  active_subscriptions_.erase(subscription_it);
  if (permission_destroyed_callback_) {
    permission_destroyed_callback_.Run(storage_domain);
  }
}

}  // namespace permissions
