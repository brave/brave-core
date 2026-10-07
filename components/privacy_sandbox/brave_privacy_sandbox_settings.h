/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
#define BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/privacy_sandbox/privacy_sandbox_settings.h"

class HostContentSettingsMap;
class PrefService;

namespace content_settings {
class CookieSettings;
}  // namespace content_settings

namespace privacy_sandbox {
class TrackingProtectionSettings;
}  // namespace privacy_sandbox

class BravePrivacySandboxSettings
    : public privacy_sandbox::PrivacySandboxSettings {
 public:
  BravePrivacySandboxSettings(
      HostContentSettingsMap* host_content_settings_map,
      content_settings::CookieSettings* cookie_settings,
      PrefService* pref_service);
  ~BravePrivacySandboxSettings() override;

  // PrivacySandboxSettings:
  bool IsEventReportingDestinationAttested(
      const url::Origin& destination_origin,
      privacy_sandbox::PrivacySandboxAttestationsGatedAPI invoking_api)
      const override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  bool AreRelatedWebsiteSetsEnabled() const override;

 private:
  // Callback to ensure we don't ever enable the Privacy Sandbox.
  void OnPrivacySandboxPrefChanged();

  raw_ptr<PrefService> pref_service_;
  PrefChangeRegistrar user_prefs_registrar_;
};

#endif  // BRAVE_COMPONENTS_PRIVACY_SANDBOX_BRAVE_PRIVACY_SANDBOX_SETTINGS_H_
