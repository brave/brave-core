/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_manager.h"

#include <algorithm>
#include <utility>

#include "base/auto_reset.h"
#include "base/containers/contains.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/pref_registry/pref_registry_syncable.h"

using content_settings::WebsiteSettingsInfo;
using content_settings::WebsiteSettingsRegistry;

namespace permissions {

// static
void PermissionLifetimeManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  // Ensure the content settings are all registered.
  content_settings::ContentSettingsRegistry::GetInstance();

  registry->RegisterDictionaryPref(prefs::kPermissionLifetimeRoot);

  PermissionExpirations::RegisterProfilePrefs(registry);
}

PermissionLifetimeManager::PermissionLifetimeManager(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs,
    std::unique_ptr<PermissionOriginLifetimeMonitor>
        permission_origin_lifetime_monitor)
    : host_content_settings_map_(host_content_settings_map),
      prefs_(prefs),
      permission_origin_lifetime_monitor_(
          std::move(permission_origin_lifetime_monitor)),
      permission_expirations_(prefs_),
      expiration_timer_(std::make_unique<util::WallClockTimer>()) {
  DCHECK(host_content_settings_map_);
  // In incognito prefs_ is nullptr.

  ResetExpiredPermissionsAndUpdateTimer(base::Time::Now());

  if (permission_origin_lifetime_monitor_) {
    ResetAllDomainPermissions();
    permission_origin_lifetime_monitor_->SetOnPermissionOriginDestroyedCallback(
        base::BindRepeating(
            &PermissionLifetimeManager::OnPermissionOriginDestroyed,
            base::Unretained(this)));
  }

  host_content_settings_map_observation_.Observe(host_content_settings_map_);
}

PermissionLifetimeManager::~PermissionLifetimeManager() {}

void PermissionLifetimeManager::Shutdown() {
  host_content_settings_map_observation_.Reset();
  permission_origin_lifetime_monitor_.reset();
  StopExpirationTimer();
}

void PermissionLifetimeManager::PermissionDecided(
    const PermissionRequest& permission_request,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSetting content_setting,
    bool is_one_time) {
  if (!permission_request.SupportsLifetime() ||
      (content_setting != ContentSetting::CONTENT_SETTING_ALLOW &&
       content_setting != ContentSetting::CONTENT_SETTING_BLOCK) ||
      is_one_time) {
    // Only interested in ALLOW/BLOCK and non one-time (Chromium
    // geolocation-specific) decisions.
    return;
  }

  const auto& lifetime = permission_request.GetLifetime();
  if (!lifetime) {
    // If no lifetime is set, then we don't need to do anything here.
    return;
  }

  const ContentSettingsType content_type =
      permission_request.GetContentSettingsType();

  DVLOG(1) << "PermissionLifetimeManager::PermissionDecided"
           << "\ntype: "
           << WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name()
           << "\nrequesting_origin: " << requesting_origin
           << "\nembedding_origin: " << embedding_origin
           << "\ncontent_setting: "
           << content_settings::ContentSettingToString(content_setting)
           << "\nlifetime: " << permission_request.GetLifetime()->InSeconds()
           << " seconds";

  if (*lifetime == base::TimeDelta()) {
    DCHECK(permission_origin_lifetime_monitor_);
    std::string key =
        permission_origin_lifetime_monitor_
            ->SubscribeToPermissionOriginDestruction(requesting_origin);
    if (key.empty()) {
      // There is no any active origin with this key, so reset the permission
      // right away. PostTask is required because at this point the permission
      // is not stored in HostContentSettingsMap yet.
      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(&PermissionLifetimeManager::ResetPermission,
                         weak_ptr_factory_.GetWeakPtr(), content_type,
                         requesting_origin, embedding_origin));
      return;
    }
    permission_expirations_.AddExpiringPermission(
        content_type, PermissionExpirationKey(std::move(key)),
        PermissionOrigins(requesting_origin, embedding_origin,
                          content_setting));
  } else {
    const base::Time expiration_time = base::Time::Now() + *lifetime;
    permission_expirations_.AddExpiringPermission(
        content_type, PermissionExpirationKey(expiration_time),
        PermissionOrigins(requesting_origin, embedding_origin,
                          content_setting));
    UpdateExpirationTimer();
  }
}

void PermissionLifetimeManager::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  DVLOG(1) << "PermissionLifetimeManager::OnContentSettingChanged"
           << "\nis_currently_removing_permissions_ "
           << is_currently_removing_permissions_ << "\ntype: "
           << WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name()
           << "\nprimary_pattern: " << primary_pattern.ToString()
           << "\nsecondary_pattern: " << secondary_pattern.ToString();

  if (is_currently_removing_permissions_) {
    return;
  }

  // Don't try to do anything if a content_type is not handled at all.
  if (!base::Contains(permission_expirations_.expirations(), content_type)) {
    return;
  }

  auto remove_predicate = base::BindRepeating(
      [](const HostContentSettingsMap* host_content_settings_map,
         const ContentSettingsPattern* primary_pattern,
         const ContentSettingsPattern* secondary_pattern,
         const ContentSettingsType content_type,
         const PermissionOrigins& origins) {
        if (primary_pattern->IsValid() &&
            !primary_pattern->Matches(origins.requesting_origin())) {
          return false;
        }
        if (secondary_pattern->IsValid() &&
            !secondary_pattern->Matches(origins.embedding_origin())) {
          return false;
        }
        return host_content_settings_map->GetContentSetting(
                   origins.requesting_origin(), origins.embedding_origin(),
                   content_type) != origins.content_setting();
      },
      base::Unretained(host_content_settings_map_),
      base::Unretained(&primary_pattern), base::Unretained(&secondary_pattern),
      content_type);

  if (!permission_expirations_.RemoveExpiringPermissions(content_type,
                                                         remove_predicate)) {
    return;
  }

  UpdateExpirationTimer();
}

void PermissionLifetimeManager::RestartExpirationTimerForTesting() {
  StopExpirationTimer();
  // Recreate timer to acknowledge a new task runnner.
  expiration_timer_ = std::make_unique<util::WallClockTimer>();
  UpdateExpirationTimer();
}

void PermissionLifetimeManager::UpdateExpirationTimer() {
  base::Time nearest_expiration_time(base::Time::Max());
  for (const auto& type_expirations : permission_expirations_.expirations()) {
    const auto& key_expirations_map = type_expirations.second;
    if (key_expirations_map.empty() ||
        !key_expirations_map.begin()->first.IsTimeKey()) {
      continue;
    }
    nearest_expiration_time = std::min(
        nearest_expiration_time, key_expirations_map.begin()->first.time());
  }

  if (nearest_expiration_time == base::Time::Max()) {
    // Nothing to wait for. Stop the timer and return.
    StopExpirationTimer();
    return;
  }

  if (current_scheduled_expiration_time_ == nearest_expiration_time) {
    // Timer is already correct. Do nothing.
    DCHECK(expiration_timer_->IsRunning());
    return;
  }

  current_scheduled_expiration_time_ = nearest_expiration_time;
  expiration_timer_->Start(
      FROM_HERE, current_scheduled_expiration_time_,
      base::BindOnce(&PermissionLifetimeManager::OnExpirationTimer,
                     base::Unretained(this)));
}

void PermissionLifetimeManager::StopExpirationTimer() {
  expiration_timer_->Stop();
  current_scheduled_expiration_time_ = base::Time();
}

void PermissionLifetimeManager::OnExpirationTimer() {
  DCHECK(!current_scheduled_expiration_time_.is_null());
  ResetExpiredPermissionsAndUpdateTimer(current_scheduled_expiration_time_);
}

void PermissionLifetimeManager::ResetExpiredPermissionsAndUpdateTimer(
    base::Time current_expiration_time) {
  base::AutoReset<bool> auto_reset(&is_currently_removing_permissions_, true);
  for (const auto& expired_permissions :
       permission_expirations_.RemoveExpiredPermissions(
           current_expiration_time)) {
    const auto& content_type = expired_permissions.first;
    const auto& expiring_permissions = expired_permissions.second;
    for (const auto& expiring_permission : expiring_permissions) {
      ResetPermission(content_type, expiring_permission.requesting_origin(),
                      expiring_permission.embedding_origin());
    }
  }
  UpdateExpirationTimer();
}

void PermissionLifetimeManager::OnPermissionOriginDestroyed(
    const std::string& origin_key) {
  base::AutoReset<bool> auto_reset(&is_currently_removing_permissions_, true);
  for (const auto& expired_permissions :
       permission_expirations_.RemoveExpiredPermissions(origin_key)) {
    const auto& content_type = expired_permissions.first;
    const auto& expiring_permissions = expired_permissions.second;
    for (const auto& expiring_permission : expiring_permissions) {
      ResetPermission(content_type, expiring_permission.requesting_origin(),
                      expiring_permission.embedding_origin());
    }
  }
}

void PermissionLifetimeManager::ResetAllDomainPermissions() {
  base::AutoReset<bool> auto_reset(&is_currently_removing_permissions_, true);
  for (const auto& expired_permissions :
       permission_expirations_.RemoveAllDomainPermissions()) {
    const auto& content_type = expired_permissions.first;
    const auto& expiring_permissions = expired_permissions.second;
    for (const auto& expiring_permission : expiring_permissions) {
      ResetPermission(content_type, expiring_permission.requesting_origin(),
                      expiring_permission.embedding_origin());
    }
  }
}

void PermissionLifetimeManager::ResetPermission(
    ContentSettingsType content_type,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  host_content_settings_map_->SetContentSettingDefaultScope(
      requesting_origin, embedding_origin, content_type,
      CONTENT_SETTING_DEFAULT);
}

}  // namespace permissions
