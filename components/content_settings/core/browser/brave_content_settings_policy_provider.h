/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_POLICY_PROVIDER_H_
#define BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_POLICY_PROVIDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/content_settings/core/browser/content_settings_policy_provider.h"

namespace content_settings {

// With this subclass, shields configuration is persisted across sessions.
class BravePolicyProvider : public PolicyProvider {
 public:
  explicit BravePolicyProvider(PrefService* prefs);
  ~BravePolicyProvider() override;

  BravePolicyProvider(const BravePolicyProvider&) = delete;
  BravePolicyProvider& operator=(const BravePolicyProvider&) = delete;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  static const PolicyProvider::PrefsForManagedDefaultMapEntry
      kBravePrefsForManagedDefault[];

  void ReadManagedContentSettings(bool overwrite) override;

  void OnPreferenceChanged(const std::string& name);
  void GetBraveContentSettingsFromPreferences(
      OriginIdentifierValueMap* value_map);
};

}  //  namespace content_settings

#endif  // BRAVE_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_BRAVE_CONTENT_SETTINGS_POLICY_PROVIDER_H_
