// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/farbling/brave_base_farbling_browsertest.h"

#include "base/token.h"
#include "brave/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"

void BraveBaseFarblingBrowserTest::SetUpOnMainThread() {
  InProcessBrowserTest::SetUpOnMainThread();
  auto* brave_settings_service =
      BraveShieldsSettingsServiceFactory::GetForProfile(browser()->profile());
  brave_settings_service->set_profile_level_farbling_entropy_for_testing(
      base::Token());
}
