/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/system_timestamp_user_data.h"

#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/time_util.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetSystemTimestamp() {
  const std::string timestamp =
      TimeToPrivacyPreservingISO8601(base::Time::Now());

  base::DictionaryValue user_data;
  user_data.SetKey("systemTimestamp", base::Value(timestamp));

  return user_data;
}

}  // namespace user_data
}  // namespace ads
