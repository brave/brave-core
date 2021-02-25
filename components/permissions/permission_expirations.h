/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATIONS_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATIONS_H_

#include <map>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "brave/components/permissions/permission_origins.h"
#include "components/content_settings/core/common/content_settings.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace permissions {

// Handles Add/Remove to a permissions expiration container and syncs it with
// prefs.
class PermissionExpirations {
 public:
  using ExpiringPermissions = std::vector<PermissionOrigins>;
  using TimeExpirationsMap = std::map<base::Time, ExpiringPermissions>;
  using TypeTimeExpirationsMap =
      base::flat_map<ContentSettingsType, TimeExpirationsMap>;
  using ExpiredPermissions =
      base::flat_map<ContentSettingsType, ExpiringPermissions>;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  explicit PermissionExpirations(PrefService* prefs);
  PermissionExpirations(const PermissionExpirations&) = delete;
  PermissionExpirations& operator=(const PermissionExpirations&) = delete;
  ~PermissionExpirations();

  // Add expiring permission.
  void AddExpiringPermission(ContentSettingsType content_type,
                             base::Time expiration_time,
                             PermissionOrigins permission_origins);
  // Remove permission using |predicate|. Returns true if anything was removed.
  bool RemoveExpiringPermissions(
      ContentSettingsType content_type,
      base::RepeatingCallback<bool(const PermissionOrigins&)> predicate);
  // Remove expired permissions with expiration_time > |current_time|.
  ExpiredPermissions RemoveExpiredPermissions(base::Time current_time);

  const TypeTimeExpirationsMap& expirations() const { return expirations_; }

 private:
  // Update value in prefs, |time_items| used to update only listed items.
  void UpdateTimeExpirationsPref(ContentSettingsType content_type,
                                 const std::vector<base::Time>& time_items);

  void ReadExpirationsFromPrefs();
  ExpiringPermissions ParseExpiringPermissions(
      const base::Value& expiring_permissions_val);
  base::Value ExpiringPermissionsToValue(
      const ExpiringPermissions& expiring_permissions) const;

  PrefService* const prefs_ = nullptr;

  // Expirations data from prefs used at runtime. Kept in sync with prefs.
  TypeTimeExpirationsMap expirations_;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_EXPIRATIONS_H_
