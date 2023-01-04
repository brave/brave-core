/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/version_number_user_data.h"

#include "bat/ads/internal/browser/browser_util.h"

namespace ads::user_data {

namespace {
constexpr char kVersionNumberKey[] = "versionNumber";
}  // namespace

base::Value::Dict GetVersionNumber() {
  base::Value::Dict user_data;

  user_data.Set(kVersionNumberKey, GetBrowserVersionNumber());

  return user_data;
}

}  // namespace ads::user_data
