/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_SETTINGS_SERVICE_H_
#define BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_SETTINGS_SERVICE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "url/origin.h"

class PrefService;

namespace psst {

class PsstSettingsService : public KeyedService {
 public:
  class PrefObserver : public base::CheckedObserver {
   public:
    virtual void OnPsstEnableChange(bool new_value) {}

   protected:
    ~PrefObserver() override = default;
  };

  explicit PsstSettingsService(
      HostContentSettingsMap& host_content_settings_map,
      PrefService* prefs);
  ~PsstSettingsService() override;

  void AddObserver(PrefObserver* observer);
  void RemoveObserver(PrefObserver* observer);

  // Saves the PSST metadata for the (origin, user_id) pair with the given
  // details.
  void SetPsstWebsiteSettings(const url::Origin& origin,
                              ConsentStatus consent_status,
                              int script_version,
                              std::string_view user_id,
                              base::ListValue urls_to_skip);

  // Returns the PSST settings for the (origin, user_id) pair if exists
  std::optional<PsstWebsiteSettings> GetPsstWebsiteSettings(
      const url::Origin& origin,
      std::string_view user_id);

  void SetPsstWebsiteSettings(const url::Origin& origin,
                              PsstWebsiteSettings psst_metadata);

  bool IsPsstEnabled() const;
  void SetPsstEnabled(bool enabled);

 private:
  void OnPreferenceChanged(const std::string& pref_name);

  const raw_ref<HostContentSettingsMap>
      host_content_settings_map_;         // NOT OWNED
  raw_ptr<PrefService> prefs_ = nullptr;  // NOT OWNED
  base::ObserverList<PrefObserver> observers_;
  PrefChangeRegistrar pref_change_registrar_;
  base::WeakPtrFactory<PsstSettingsService> weak_ptr_factory_{this};
};

}  // namespace psst

#endif  // BRAVE_COMPONENTS_PSST_CORE_BROWSER_PSST_SETTINGS_SERVICE_H_
