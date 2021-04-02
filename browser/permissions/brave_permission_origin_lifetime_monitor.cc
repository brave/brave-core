/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_permission_origin_lifetime_monitor.h"

#include <utility>

#include "base/stl_util.h"
#include "content/public/browser/tld_ephemeral_lifetime.h"
#include "net/base/features.h"
#include "net/base/url_util.h"

namespace permissions {

BravePermissionOriginLifetimeMonitor::BravePermissionOriginLifetimeMonitor(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
  DCHECK(browser_context_);
  DCHECK(base::FeatureList::IsEnabled(net::features::kBraveEphemeralStorage));
}

BravePermissionOriginLifetimeMonitor::~BravePermissionOriginLifetimeMonitor() =
    default;

void BravePermissionOriginLifetimeMonitor::
    SetOnPermissionOriginDestroyedCallback(
        base::RepeatingCallback<void(const std::string&)> callback) {
  permission_destroyed_callback_ = std::move(callback);
}

std::string
BravePermissionOriginLifetimeMonitor::SubscribeToPermissionOriginDestruction(
    const GURL& requesting_origin) {
  DCHECK(permission_destroyed_callback_);
  std::string storage_domain =
      net::URLToEphemeralStorageDomain(requesting_origin);
  auto* tld_ephemeral_lifetime =
      content::TLDEphemeralLifetime::Get(browser_context_, storage_domain);
  if (!tld_ephemeral_lifetime) {
    // If an ephemeral lifetime object doesn't exist, treat a permission origin
    // as an already destroyed one.
    return std::string();
  }

  if (!base::Contains(active_subscriptions_, storage_domain)) {
    tld_ephemeral_lifetime->RegisterOnDestroyCallback(base::BindOnce(
        &BravePermissionOriginLifetimeMonitor::OnEphemeralTLDDestroyed,
        weak_ptr_factory_.GetWeakPtr()));
    active_subscriptions_.insert(storage_domain);
  }
  return storage_domain;
}

void BravePermissionOriginLifetimeMonitor::OnEphemeralTLDDestroyed(
    const std::string& storage_domain) {
  DCHECK(base::Contains(active_subscriptions_, storage_domain));
  active_subscriptions_.erase(storage_domain);
  if (permission_destroyed_callback_) {
    permission_destroyed_callback_.Run(storage_domain);
  }
}

}  // namespace permissions
