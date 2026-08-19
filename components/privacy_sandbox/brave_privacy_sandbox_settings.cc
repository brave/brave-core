/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

#include <string>

#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_prefs.h"

BravePrivacySandboxSettings::BravePrivacySandboxSettings(
    HostContentSettingsMap* host_content_settings_map,
    content_settings::CookieSettings* cookie_settings,
    PrefService* pref_service)
    : pref_service_(pref_service) {
  // Register observers for the Privacy Sandbox.
  user_prefs_registrar_.Init(pref_service_);
  user_prefs_registrar_.Add(
      prefs::kPrivacySandboxRelatedWebsiteSetsEnabled,
      base::BindRepeating(
          &BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged,
          base::Unretained(this)));
}

BravePrivacySandboxSettings::~BravePrivacySandboxSettings() = default;

void BravePrivacySandboxSettings::OnPrivacySandboxPrefChanged() {
  // Make sure that Private Sandbox features remain disabled even if we manually
  // access the Pref service and try to change the preferences from there.
  if (pref_service_->GetBoolean(
          prefs::kPrivacySandboxRelatedWebsiteSetsEnabled)) {
    pref_service_->SetBoolean(prefs::kPrivacySandboxRelatedWebsiteSetsEnabled,
                              false);
  }
}

// PrivacySandboxSettings:

bool BravePrivacySandboxSettings::IsEventReportingDestinationAttested(
    const url::Origin& destination_origin,
    privacy_sandbox::PrivacySandboxAttestationsGatedAPI invoking_api) const {
  return false;
}

bool BravePrivacySandboxSettings::IsSharedStorageAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& accessing_origin,
    std::string* out_debug_message,
    content::RenderFrameHost* console_frame,
    bool* out_block_is_site_setting_specific) const {
  return false;
}

bool BravePrivacySandboxSettings::IsSharedStorageSelectURLAllowed(
    const url::Origin& top_frame_origin,
    const url::Origin& accessing_origin,
    std::string* out_debug_message,
    bool* out_block_is_site_setting_specific) const {
  return false;
}

void BravePrivacySandboxSettings::AddObserver(Observer* observer) {}
void BravePrivacySandboxSettings::RemoveObserver(Observer* observer) {}

bool BravePrivacySandboxSettings::AreRelatedWebsiteSetsEnabled() const {
  return false;
}
