/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_IMPL_H_
#define BRAVE_BROWSER_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_IMPL_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_service_observer.h"
#include "brave/components/permissions/permission_origin_lifetime_monitor.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace ephemeral_storage {
class EphemeralStorageService;
}  // namespace ephemeral_storage

namespace permissions {

// Uses TLDEphemeralLifetime to observe a permission origin destruction.
class PermissionOriginLifetimeMonitorImpl
    : public PermissionOriginLifetimeMonitor,
      public ephemeral_storage::EphemeralStorageServiceObserver {
 public:
  explicit PermissionOriginLifetimeMonitorImpl(
      content::BrowserContext* browser_context);
  PermissionOriginLifetimeMonitorImpl(
      const PermissionOriginLifetimeMonitorImpl&) = delete;
  PermissionOriginLifetimeMonitorImpl& operator=(
      const PermissionOriginLifetimeMonitorImpl&) = delete;
  ~PermissionOriginLifetimeMonitorImpl() override;

  void SetOnPermissionOriginDestroyedCallback(
      base::RepeatingCallback<void(const std::string&)> callback) override;
  std::string SubscribeToPermissionOriginDestruction(
      const GURL& requesting_origin) override;

 private:
  // ephemeral_storage::EphemeralStorageServiceObserver:
  void OnCleanupTLDEphemeralArea(
      const ephemeral_storage::TLDEphemeralAreaKey& key) override;

  const raw_ptr<content::BrowserContext> browser_context_;

  base::ScopedObservation<ephemeral_storage::EphemeralStorageService,
                          ephemeral_storage::EphemeralStorageServiceObserver>
      ephemeral_storage_observation_{this};
  base::RepeatingCallback<void(const std::string&)>
      permission_destroyed_callback_;
  base::flat_set<std::string> active_subscriptions_;

  base::WeakPtrFactory<PermissionOriginLifetimeMonitorImpl> weak_ptr_factory_{
      this};
};

}  // namespace permissions

#endif  // BRAVE_BROWSER_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_IMPL_H_
