/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/platform_user_data.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace ads::user_data {

namespace {
constexpr char kPlatformKey[] = "platform";
}  // namespace

base::Value::Dict GetPlatform() {
  base::Value::Dict user_data;

  const std::string platform_name = PlatformHelper::GetInstance()->GetName();
  if (!platform_name.empty()) {
    user_data.Set(kPlatformKey, platform_name);
  }

  return user_data;
}

}  // namespace ads::user_data
