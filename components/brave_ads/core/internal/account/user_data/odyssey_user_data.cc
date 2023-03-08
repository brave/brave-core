/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/odyssey_user_data.h"

#include <string>

#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/sys_info.h"

namespace ads::user_data {

namespace {

constexpr char kOdysseyKey[] = "odyssey";
constexpr char kGuest[] = "guest";
constexpr char kHost[] = "host";

}  // namespace

base::Value::Dict GetOdyssey() {
  base::Value::Dict user_data;

  const std::string type = SysInfo().is_uncertain_future ? kGuest : kHost;
  user_data.Set(kOdysseyKey, type);

  return user_data;
}

}  // namespace ads::user_data
