/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser/browser_util.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/version_info/version_info.h"

namespace ads {

std::string GetBrowserVersionNumber() {
  return version_info::GetBraveChromiumVersionNumber();
}

bool WasBrowserUpgraded() {
  static absl::optional<bool> was_upgraded;

  if (was_upgraded) {
    return *was_upgraded;
  }

  const std::string version_number = GetBrowserVersionNumber();

  const std::string last_version_number =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kBrowserVersionNumber);

  was_upgraded = version_number != last_version_number;

  if (was_upgraded) {
    AdsClientHelper::GetInstance()->SetStringPref(prefs::kBrowserVersionNumber,
                                                  version_number);
  }

  return *was_upgraded;
}

}  // namespace ads
