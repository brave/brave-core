/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings.h"

#define OnPrivacySandboxPrefChanged() OnPrivacySandboxPrefChanged_ChromiumImpl()

#include "../../../../../chrome/browser/privacy_sandbox/privacy_sandbox_settings.cc"  // NOLINT

#undef OnPrivacySandboxPrefChanged

void PrivacySandboxSettings::OnPrivacySandboxPrefChanged() {
  OnPrivacySandboxPrefChanged_ChromiumImpl();

  // Make sure that Private Sandbox features remain disabled even if we manually
  // access the Pref service and try to change the preference from there.
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxApisEnabled))
    pref_service_->SetBoolean(prefs::kPrivacySandboxApisEnabled, false);
  if (pref_service_->GetBoolean(prefs::kPrivacySandboxFlocEnabled))
    pref_service_->SetBoolean(prefs::kPrivacySandboxFlocEnabled, false);
}
