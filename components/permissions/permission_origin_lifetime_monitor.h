/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_

#include <string>

#include "base/functional/callback.h"
#include "url/gurl.h"

namespace permissions {

// Helper to support an origin-based permission lifetime logic.
class PermissionOriginLifetimeMonitor {
 public:
  virtual ~PermissionOriginLifetimeMonitor() = default;

  // Set a callback to call when a permission origin is destroyed.
  // Callback will receive a string returned by
  // |SubscribeToPermissionOriginDestruction|.
  virtual void SetOnPermissionOriginDestroyedCallback(
      base::RepeatingCallback<void(const std::string&)> callback) = 0;

  // Subscribe to a permission origin destruction. Returns an ephemeral storage
  // domain or an empty string if a storage partition for |requesting_origin|
  // doesn't exist. Returned string will be used in a callback.
  virtual std::string SubscribeToPermissionOriginDestruction(
      const GURL& requesting_origin) = 0;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_ORIGIN_LIFETIME_MONITOR_H_
