/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/odyssey_user_data.h"

#include <string>

#include "base/values.h"
#include "bat/ads/ads.h"

namespace ads {
namespace user_data {

namespace {

constexpr char kOdysseyKey[] = "odyssey";
constexpr char kGuest[] = "guest";
constexpr char kHost[] = "host";

}  // namespace

base::DictionaryValue GetOdyssey() {
  base::DictionaryValue user_data;

  const std::string type = SysInfo().is_uncertain_future ? kGuest : kHost;
  user_data.SetStringKey(kOdysseyKey, type);

  return user_data;
}

}  // namespace user_data
}  // namespace ads
