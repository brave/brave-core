/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

#define BuildServiceInstanceFor BuildServiceInstanceFor_ChromiumImpl
#include "src/chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.cc"
#undef BuildServiceInstanceFor

KeyedService* PrivacySandboxSettingsFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  return new BravePrivacySandboxSettings(
      HostContentSettingsMapFactory::GetForProfile(profile),
      CookieSettingsFactory::GetForProfile(profile).get(), profile->GetPrefs(),
      profile->IsIncognitoProfile());
}
