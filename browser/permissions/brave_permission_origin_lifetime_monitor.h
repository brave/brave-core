/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_BRAVE_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_
#define BRAVE_BROWSER_PERMISSIONS_BRAVE_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/permissions/permission_origin_lifetime_monitor.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace permissions {

// Uses TLDEphemeralLifetime to observe a permission origin destruction.
class BravePermissionOriginLifetimeMonitor
    : public PermissionOriginLifetimeMonitor {
 public:
  BravePermissionOriginLifetimeMonitor(
      content::BrowserContext* browser_context);
  BravePermissionOriginLifetimeMonitor(
      const BravePermissionOriginLifetimeMonitor&) = delete;
  BravePermissionOriginLifetimeMonitor& operator=(
      const BravePermissionOriginLifetimeMonitor&) = delete;
  ~BravePermissionOriginLifetimeMonitor() override;

  void SetOnPermissionOriginDestroyedCallback(
      base::RepeatingCallback<void(const std::string&)> callback) override;
  // Returns an ephemeral storage domain or an empty string if a storage
  // partition for |requesting_origin| doesn't exist.
  std::string SubscribeToPermissionOriginDestruction(
      const GURL& requesting_origin) override;

 private:
  void OnEphemeralTLDDestroyed(const std::string& storage_domain);

  content::BrowserContext* const browser_context_ = nullptr;

  base::RepeatingCallback<void(const std::string&)>
      permission_destroyed_callback_;
  base::flat_set<std::string> active_subscriptions_;

  base::WeakPtrFactory<BravePermissionOriginLifetimeMonitor> weak_ptr_factory_{
      this};
};

}  // namespace permissions

#endif  // BRAVE_BROWSER_PERMISSIONS_BRAVE_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_
