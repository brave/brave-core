/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/version_number_user_data.h"

#include "brave/components/brave_ads/core/internal/browser/browser_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr char kVersionNumberKey[] = "versionNumber";
}  // namespace

base::Value::Dict BuildVersionNumberUserData() {
  base::Value::Dict user_data;

  if (!UserHasJoinedBraveRewards()) {
    return user_data;
  }

  user_data.Set(kVersionNumberKey, GetBrowserVersionNumber());

  return user_data;
}

}  // namespace brave_ads
