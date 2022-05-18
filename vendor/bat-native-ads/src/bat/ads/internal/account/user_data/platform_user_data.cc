/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/platform_user_data.h"

#include <string>

#include "base/values.h"
#include "bat/ads/internal/base/platform_helper.h"

namespace ads {
namespace user_data {

namespace {
constexpr char kPlatformKey[] = "platform";
}  // namespace

base::DictionaryValue GetPlatform() {
  base::DictionaryValue user_data;

  const std::string platform_name = PlatformHelper::GetInstance()->GetName();
  if (!platform_name.empty()) {
    user_data.SetStringKey(kPlatformKey, platform_name);
  }

  return user_data;
}

}  // namespace user_data
}  // namespace ads
