/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/permission_origin_lifetime_monitor_impl.h"

#include <utility>

#include "base/containers/contains.h"
#include "brave/browser/ephemeral_storage/tld_ephemeral_lifetime.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
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
  auto* tld_ephemeral_lifetime = ephemeral_storage::TLDEphemeralLifetime::Get(
      browser_context_, storage_domain);
  if (!tld_ephemeral_lifetime) {
    DCHECK(!base::Contains(active_subscriptions_, storage_domain));
    // If an ephemeral lifetime object doesn't exist, treat a permission origin
    // as an already destroyed one.
    return std::string();
  }

  if (!base::Contains(active_subscriptions_, storage_domain)) {
    tld_ephemeral_lifetime->RegisterOnDestroyCallback(base::BindOnce(
        &PermissionOriginLifetimeMonitorImpl::OnEphemeralTLDDestroyed,
        weak_ptr_factory_.GetWeakPtr()));
    active_subscriptions_.insert(storage_domain);
  }
  return storage_domain;
}

void PermissionOriginLifetimeMonitorImpl::OnEphemeralTLDDestroyed(
    const std::string& storage_domain) {
  DCHECK(base::Contains(active_subscriptions_, storage_domain));
  active_subscriptions_.erase(storage_domain);
  if (permission_destroyed_callback_) {
    permission_destroyed_callback_.Run(storage_domain);
  }
}

}  // namespace permissions
