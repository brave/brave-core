/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_platform_dto_user_data.h"

#include <string>

#include "base/values.h"
#include "bat/ads/internal/platform/platform_helper.h"

namespace ads {
namespace dto {
namespace user_data {

base::DictionaryValue GetPlatform() {
  base::DictionaryValue user_data;

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();
  if (!platform.empty()) {
    user_data.SetKey("platform", base::Value(platform));
  }

  return user_data;
}

}  // namespace user_data
}  // namespace dto
}  // namespace ads
