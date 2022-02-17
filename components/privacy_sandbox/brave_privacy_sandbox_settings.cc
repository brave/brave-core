/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"

BravePrivacySandboxSettings::BravePrivacySandboxSettings(
    HostContentSettingsMap* host_content_settings_map,
    content_settings::CookieSettings* cookie_settings,
    PrefService* pref_service)
    : PrivacySandboxSettings(host_content_settings_map,
                             cookie_settings,
                             pref_service),
      pref_service_(pref_service) {
  // Register observers for the Privacy Sandbox & FLoC preferences.
  user_prefs_registrar_.Init(pref_service_);
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxApisEnabled,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxFlocEnabled,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
}

BravePrivacySandboxSettings::~BravePrivacySandboxSettings() = default;

void BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged() {
  // Make sure that Private Sandbox features remain disabled even if we manually
  // access the Pref service and try to change the preference from there.
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxApisEnabled)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxApisEnabled, false);
  }
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxFlocEnabled)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxFlocEnabled, false);
  }
}
