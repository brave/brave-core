/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_

#include <map>
#include <memory>

#include "base/scoped_observation.h"
#include "base/util/timer/wall_clock_timer.h"
#include "brave/components/permissions/permission_expirations.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/permissions/permission_util.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace permissions {

// Keeps permission expirations and resets permissions when a lifetime is
// expired.
class PermissionLifetimeManager : public KeyedService,
                                  public content_settings::Observer {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  PermissionLifetimeManager(HostContentSettingsMap* host_content_settings_map,
                            PrefService* prefs);
  PermissionLifetimeManager(const PermissionLifetimeManager&) = delete;
  PermissionLifetimeManager& operator=(const PermissionLifetimeManager&) =
      delete;
  ~PermissionLifetimeManager() override;

  // KeyedService:
  void Shutdown() override;

  // Saves permission lifetime to prefs and restarts expiration timer if
  // required.
  void PermissionDecided(const PermissionRequest& permission_request,
                         const GURL& requesting_origin,
                         const GURL& embedding_origin,
                         ContentSetting content_setting,
                         bool is_one_time);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  void RestartExpirationTimerForTesting();

 private:
  friend class PermissionLifetimeManagerBrowserTest;
  friend class PermissionLifetimeManagerTest;

  // Restarts/stops Expiration timer if required.
  void UpdateExpirationTimer();
  // Stops Expiration timer.
  void StopExpirationTimer();
  // Resets permission, updates prefs and restarts Expiration timer if required.
  void OnExpirationTimer();
  void ResetExpiredPermissionsAndUpdateTimer(
      base::Time current_expiration_time);

  HostContentSettingsMap* const host_content_settings_map_ = nullptr;
  PrefService* const prefs_ = nullptr;
  // Expirations data from prefs used at runtime. Kept in sync with prefs.
  PermissionExpirations permission_expirations_;
  // WallClockTimer to reset permissions properly even if a machine was put in
  // a long sleep/wake cycle. Stored as pointer to recreate in tests.
  std::unique_ptr<util::WallClockTimer> expiration_timer_;

  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      host_content_settings_map_observation_{this};

  // If an expiration timer is running, here is stored an expiration of the most
  // recent permission to expire.
  base::Time current_scheduled_expiration_time_;
  // Flag to ignore notifications from HostContentSettingsMap when a permission
  // reset is in progress.
  bool is_currently_removing_permissions_ = false;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_
