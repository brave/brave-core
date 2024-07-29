/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

#include <optional>

#include "base/check_is_test.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/version_info/version_info.h"

namespace brave_ads {

namespace {

bool g_browser_version_number_for_testing = false;

const char kBrowserVersionNumberForTesting[] = "1.2.3.4";

}  // namespace

std::string GetBrowserVersionNumber() {
  if (g_browser_version_number_for_testing) {
    CHECK_IS_TEST();

    return kBrowserVersionNumberForTesting;
  }

  return version_info::GetBraveChromiumVersionNumber();
}

ScopedBrowserVersionNumberForTesting::ScopedBrowserVersionNumberForTesting() {
  CHECK_IS_TEST();

  g_browser_version_number_for_testing = true;
}

ScopedBrowserVersionNumberForTesting::~ScopedBrowserVersionNumberForTesting() {
  g_browser_version_number_for_testing = false;
}

bool WasBrowserUpgraded() {
  static std::optional<bool> was_upgraded;

  if (was_upgraded) {
    return *was_upgraded;
  }

  const std::string browser_version_number = GetBrowserVersionNumber();

  const std::string last_browser_version_number =
      GetProfileStringPref(prefs::kBrowserVersionNumber);

  was_upgraded = browser_version_number != last_browser_version_number;

  if (was_upgraded) {
    SetProfileStringPref(prefs::kBrowserVersionNumber, browser_version_number);
  }

  return *was_upgraded;
}

}  // namespace brave_ads
