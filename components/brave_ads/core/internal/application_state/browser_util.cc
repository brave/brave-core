/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/version_info/version_info.h"

namespace brave_ads {

namespace {

bool g_browser_version_for_testing = false;

const char kBrowserVersionForTesting[] = "1.2.3.4";

}  // namespace

std::string GetBrowserVersionNumber() {
  if (g_browser_version_for_testing) {
    return kBrowserVersionForTesting;
  }

  return version_info::GetBraveChromiumVersionNumber();
}

ScopedBrowserVersionSetterForTesting::ScopedBrowserVersionSetterForTesting() {
  g_browser_version_for_testing = true;
}

ScopedBrowserVersionSetterForTesting::~ScopedBrowserVersionSetterForTesting() {
  g_browser_version_for_testing = false;
}

bool WasBrowserUpgraded() {
  static std::optional<bool> was_upgraded;

  if (was_upgraded) {
    return *was_upgraded;
  }

  const std::string version_number = GetBrowserVersionNumber();

  const std::string last_version_number =
      GetProfileStringPref(prefs::kBrowserVersionNumber);

  was_upgraded = version_number != last_version_number;

  if (was_upgraded) {
    SetProfileStringPref(prefs::kBrowserVersionNumber, version_number);
  }

  return *was_upgraded;
}

}  // namespace brave_ads
