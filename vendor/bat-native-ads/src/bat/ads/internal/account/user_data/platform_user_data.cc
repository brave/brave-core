/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/platform_user_data.h"

#include <string>

#include "base/values.h"
#include "bat/ads/internal/platform/platform_helper.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetPlatform() {
  base::DictionaryValue user_data;

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();
  if (!platform.empty()) {
    user_data.SetStringKey("platform", platform);
  }

  return user_data;
}

}  // namespace user_data
}  // namespace ads
