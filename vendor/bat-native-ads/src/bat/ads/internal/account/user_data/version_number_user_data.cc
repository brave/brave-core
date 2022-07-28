/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/version_number_user_data.h"

#include "brave/components/version_info/version_info.h"

namespace ads {
namespace user_data {

namespace {
constexpr char kVersionNumberKey[] = "versionNumber";
}  // namespace

base::Value::Dict GetVersionNumber() {
  base::Value::Dict user_data;
  user_data.Set(kVersionNumberKey,
                version_info::GetBraveChromiumVersionNumber());

  return user_data;
}

}  // namespace user_data
}  // namespace ads
