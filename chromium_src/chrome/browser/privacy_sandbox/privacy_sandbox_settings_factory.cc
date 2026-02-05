/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.h"

#include "brave/components/privacy_sandbox/brave_privacy_sandbox_settings.h"

#define BuildServiceInstanceForBrowserContext \
  BuildServiceInstanceForBrowserContext_ChromiumImpl
#include <chrome/browser/privacy_sandbox/privacy_sandbox_settings_factory.cc>
#undef BuildServiceInstanceForBrowserContext

std::unique_ptr<KeyedService>
PrivacySandboxSettingsFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  return std::make_unique<BravePrivacySandboxSettings>(
      nullptr /*delegate*/, nullptr /*host_content_settings_map*/,
      nullptr /*cookie_settings*/, profile->GetPrefs());
}
