/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/rotating_hash_user_data.h"

#include "base/hash/hash.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/ads.h"

namespace ads {
namespace user_data {

namespace {
constexpr char kRotatingHashKey[] = "rotating_hash";
}  // namespace

base::DictionaryValue GetRotatingHash(const std::string& creative_instance_id) {
  base::DictionaryValue user_data;

  const std::string& device_id = SysInfo().device_id;
  if (device_id.empty()) {
    return user_data;
  }

  const int64_t timestamp_rounded_to_nearest_hour = static_cast<int64_t>(
      base::Time::Now().ToDoubleT() / base::Time::kSecondsPerHour);
  const std::string timestamp =
      base::NumberToString(timestamp_rounded_to_nearest_hour);

  const std::string rotating_hash = base::NumberToString(
      base::Hash(base::StrCat({device_id, creative_instance_id, timestamp})));
  user_data.SetStringKey(kRotatingHashKey, rotating_hash);

  return user_data;
}

}  // namespace user_data
}  // namespace ads
