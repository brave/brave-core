/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

#include <optional>

#include "base/check_is_test.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_version.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

namespace {
std::optional<bool> g_was_browser_upgraded;
}  // namespace

std::string GetBrowserVersionNumber() {
  return BrowserVersion::GetInstance().GetNumber();
}

bool WasBrowserUpgraded() {
  if (g_was_browser_upgraded) {
    return *g_was_browser_upgraded;
  }

  const std::string browser_version_number = GetBrowserVersionNumber();

  const std::string last_browser_version_number =
      GetProfileStringPref(prefs::kBrowserVersionNumber);

  g_was_browser_upgraded =
      browser_version_number != last_browser_version_number;

  if (*g_was_browser_upgraded) {
    SetProfileStringPref(prefs::kBrowserVersionNumber, browser_version_number);
  }

  return *g_was_browser_upgraded;
}

void ResetBrowserUpgradeCacheForTesting() {  // IN-TEST
  CHECK_IS_TEST();

  g_was_browser_upgraded.reset();
}

}  // namespace brave_ads
