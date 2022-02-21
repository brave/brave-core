/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
#define BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_

#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/privacy_sandbox/privacy_sandbox_settings.h"

class HostContentSettingsMap;
class PrefService;

namespace content_settings {
class CookieSettings;
}

class BravePrivacySandboxSettings : public PrivacySandboxSettings {
 public:
  BravePrivacySandboxSettings(HostContentSettingsMap* host_content_settings_map,
                              content_settings::CookieSettings* cookie_settings,
                              PrefService* pref_service,
                              bool incognito_profile);
  ~BravePrivacySandboxSettings() override;

 private:
  // Callback to ensure we don't ever enable the Privacy Sandbox.
  void OnPrivacySandboxPrefChanged();

  raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar user_prefs_registrar_;
};

#endif  // BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
