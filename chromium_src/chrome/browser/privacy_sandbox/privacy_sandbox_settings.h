/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_H_

#define OnPrivacySandboxPrefChanged() \
  OnPrivacySandboxPrefChanged();      \
  void OnPrivacySandboxPrefChanged_ChromiumImpl()

#include "../../../../../chrome/browser/privacy_sandbox/privacy_sandbox_settings.h"  // NOLINT

#undef OnPrivacySandboxPrefChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PRIVACY_SANDBOX_PRIVACY_SANDBOX_SETTINGS_H_
