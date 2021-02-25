/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_expirations.h"

#include <algorithm>

#include "base/stl_util.h"
#include "base/strings/strcat.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

using content_settings::WebsiteSettingsInfo;
using content_settings::WebsiteSettingsRegistry;

namespace permissions {

namespace {

// Pref data keys.
constexpr base::StringPiece kRequestingOriginKey = "ro";
constexpr base::StringPiece kEmbeddingOriginKey = "eo";

base::Time ParseExpirationTime(const std::string& time_str) {
  int64_t expiration_time = 0;
  if (!base::StringToInt64(time_str, &expiration_time)) {
    return base::Time();
  }
  return base::Time::FromDeltaSinceWindowsEpoch(
      base::TimeDelta::FromMicroseconds(expiration_time));
}

std::string ExpirationTimeToStr(base::Time expiration_time) {
  return std::to_string(
      expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
}

}  // namespace

// static
void PermissionExpirations::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  // Ensure the content settings are all registered.
  content_settings::ContentSettingsRegistry::GetInstance();

  registry->RegisterDictionaryPref(prefs::kPermissionLifetimeExpirations);
}

PermissionExpirations::PermissionExpirations(PrefService* prefs)
    : prefs_(prefs) {
  ReadExpirationsFromPrefs();
}

PermissionExpirations::~PermissionExpirations() = default;

void PermissionExpirations::AddExpiringPermission(
    ContentSettingsType content_type,
    base::Time expiration_time,
    PermissionOrigins permission_origins) {
  expirations_[content_type][expiration_time].push_back(
      std::move(permission_origins));
  UpdateTimeExpirationsPref(content_type, {expiration_time});
}

bool PermissionExpirations::RemoveExpiringPermissions(
    ContentSettingsType content_type,
    base::RepeatingCallback<bool(const PermissionOrigins&)> predicate) {
  auto expirations_it = expirations_.find(content_type);
  if (expirations_it == expirations_.end()) {
    return false;
  }

  auto& time_expirations_map = expirations_it->second;
  std::vector<base::Time> time_items_to_update_prefs;

  // Remove all elements for which |predicate| returned true.
  for (auto time_expirations_it = time_expirations_map.begin();
       time_expirations_it != time_expirations_map.end();) {
    const auto& expiration_time = time_expirations_it->first;
    auto& expiring_permissions = time_expirations_it->second;
    bool is_anything_removed = false;

    for (auto expiring_permission_it = expiring_permissions.begin();
         expiring_permission_it != expiring_permissions.end();) {
      if (predicate.Run(*expiring_permission_it)) {
        expiring_permission_it =
            expiring_permissions.erase(expiring_permission_it);
        is_anything_removed = true;
      } else {
        ++expiring_permission_it;
      }
    }

    // Track removed items to update prefs.
    if (is_anything_removed) {
      time_items_to_update_prefs.push_back(expiration_time);
    }

    // Remove empty nested containers.
    if (expiring_permissions.empty()) {
      time_expirations_it = time_expirations_map.erase(time_expirations_it);
    } else {
      ++time_expirations_it;
    }
  }

  // If nothing was removed then we're done here.
  if (time_items_to_update_prefs.empty()) {
    return false;
  }

  // Remove empty nested containers.
  if (time_expirations_map.empty()) {
    expirations_.erase(expirations_it);
  }

  // Update prefs.
  UpdateTimeExpirationsPref(content_type, time_items_to_update_prefs);
  return true;
}

PermissionExpirations::ExpiredPermissions
PermissionExpirations::RemoveExpiredPermissions(base::Time current_time) {
  ExpiredPermissions expired_permissions;

  // Enumerate all content types and remove all expired permissions.
  for (auto expirations_it = expirations_.begin();
       expirations_it != expirations_.end();) {
    const auto content_type = expirations_it->first;
    auto& time_expirations_map = expirations_it->second;

    std::vector<base::Time> time_items_to_clear_prefs;
    auto time_expirations_it = time_expirations_map.begin();
    for (; time_expirations_it != time_expirations_map.end();
         ++time_expirations_it) {
      const auto& expiration_time = time_expirations_it->first;
      auto& expiring_permissions = time_expirations_it->second;
      if (expiration_time > current_time) {
        // If we encountered an expiration that is still active, then all next
        // expirations will also be active (map is sorted).
        break;
      }
      expired_permissions[content_type] = std::move(expiring_permissions);
      time_items_to_clear_prefs.push_back(expiration_time);
    }
    time_expirations_map.erase(time_expirations_map.begin(),
                               time_expirations_it);

    // Remove empty nested containers.
    if (time_expirations_map.empty()) {
      expirations_it = expirations_.erase(expirations_it);
    } else {
      ++expirations_it;
    }

    // Update prefs.
    UpdateTimeExpirationsPref(content_type, time_items_to_clear_prefs);
  }
  return expired_permissions;
}

void PermissionExpirations::UpdateTimeExpirationsPref(
    ContentSettingsType content_type,
    const std::vector<base::Time>& time_items) {
  if (!prefs_) {
    return;
  }

  // Use a scoped pref update to update only changed pref subkeys.
  ::prefs::ScopedDictionaryPrefUpdate update(
      prefs_, prefs::kPermissionLifetimeExpirations);
  std::unique_ptr<::prefs::DictionaryValueUpdate> time_expirations_val =
      update.Get();
  DCHECK(time_expirations_val);

  const std::string& content_type_name =
      WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name();

  const auto& expirations_it = expirations_.find(content_type);
  if (expirations_it == expirations_.end()) {
    // Remove content type if it's absent in a runtime container.
    time_expirations_val->RemovePath(content_type_name, nullptr);
    return;
  }

  const auto& time_expirations_map = expirations_it->second;
  for (const auto& time_to_update : time_items) {
    const auto& time_expirations_it = time_expirations_map.find(time_to_update);
    if (time_expirations_it == time_expirations_map.end() ||
        time_expirations_it->second.empty()) {
      // Remove a time element if it's absent or empty in a runtime container.
      time_expirations_val->RemovePath(
          base::StrCat(
              {content_type_name, ".", ExpirationTimeToStr(time_to_update)}),
          nullptr);
    } else {
      // Update a time element if it's not empty in a runtime container.
      time_expirations_val->SetPath(
          {content_type_name, ExpirationTimeToStr(time_to_update)},
          ExpiringPermissionsToValue(time_expirations_it->second));
    }
  }
}

void PermissionExpirations::ReadExpirationsFromPrefs() {
  if (!prefs_) {
    return;
  }

  const base::Value* type_expirations_map_val =
      prefs_->GetDictionary(prefs::kPermissionLifetimeExpirations);
  DCHECK(type_expirations_map_val);
  DCHECK(type_expirations_map_val->is_dict());

  std::vector<std::string> invalid_content_type_names;
  for (const auto& type_expirations_val :
       type_expirations_map_val->DictItems()) {
    const std::string& content_type_name = type_expirations_val.first;
    const base::Value& time_expirations_map_val = type_expirations_val.second;
    if (!time_expirations_map_val.is_dict()) {
      continue;
    }
    const WebsiteSettingsInfo* website_settings_info =
        WebsiteSettingsRegistry::GetInstance()->GetByName(content_type_name);
    if (!website_settings_info) {
      invalid_content_type_names.push_back(content_type_name);
      continue;
    }
    TimeExpirationsMap time_expirations_map;
    for (const auto& time_expiring_permissions_val :
         time_expirations_map_val.DictItems()) {
      const std::string& time_str = time_expiring_permissions_val.first;
      const base::Value& expiring_permissions_val =
          time_expiring_permissions_val.second;
      const base::Time expiration_time = ParseExpirationTime(time_str);
      if (expiration_time.is_null()) {
        continue;
      }
      ExpiringPermissions expiring_permissions =
          ParseExpiringPermissions(expiring_permissions_val);
      if (expiring_permissions.empty()) {
        continue;
      }
      time_expirations_map.emplace(expiration_time,
                                   std::move(expiring_permissions));
    }
    if (!time_expirations_map.empty()) {
      expirations_.emplace(website_settings_info->type(),
                           std::move(time_expirations_map));
    }
  }

  if (!invalid_content_type_names.empty()) {
    ::prefs::ScopedDictionaryPrefUpdate update(
        prefs_, prefs::kPermissionLifetimeExpirations);
    std::unique_ptr<::prefs::DictionaryValueUpdate> time_expirations_val =
        update.Get();
    for (const auto& invalid_content_type_name : invalid_content_type_names) {
      time_expirations_val->RemovePath(invalid_content_type_name, nullptr);
    }
  }
}

PermissionExpirations::ExpiringPermissions
PermissionExpirations::ParseExpiringPermissions(
    const base::Value& expiring_permissions_val) {
  ExpiringPermissions expiring_permissions;
  if (!expiring_permissions_val.is_list()) {
    return expiring_permissions;
  }

  expiring_permissions.reserve(expiring_permissions_val.GetList().size());
  for (const auto& item : expiring_permissions_val.GetList()) {
    if (!item.is_dict()) {
      continue;
    }
    const std::string* requesting_origin =
        item.FindStringKey(kRequestingOriginKey);
    const std::string* embedding_origin =
        item.FindStringKey(kEmbeddingOriginKey);
    if (!requesting_origin) {
      continue;
    }
    expiring_permissions.push_back(
        PermissionOrigins(requesting_origin, embedding_origin));
  }

  return expiring_permissions;
}

base::Value PermissionExpirations::ExpiringPermissionsToValue(
    const ExpiringPermissions& expiring_permissions) const {
  base::Value::ListStorage items;
  items.reserve(expiring_permissions.size());

  for (const auto& expiring_permission : expiring_permissions) {
    base::Value value(base::Value::Type::DICTIONARY);
    value.SetStringKey(kRequestingOriginKey,
                       expiring_permission.requesting_origin().spec());
    if (expiring_permission.embedding_origin() !=
        expiring_permission.requesting_origin()) {
      value.SetStringKey(kEmbeddingOriginKey,
                         expiring_permission.embedding_origin().spec());
    }
    items.push_back(std::move(value));
  }

  return base::Value(std::move(items));
}

}  // namespace permissions
