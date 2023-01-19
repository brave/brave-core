/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/build_channel_user_data.h"

#include "base/check.h"
#include "bat/ads/build_channel.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::user_data {

namespace {
constexpr char kBuildChannelKey[] = "buildChannel";
}  // namespace

base::Value::Dict GetBuildChannel() {
  base::Value::Dict user_data;

  DCHECK(!BuildChannel().name.empty());
  user_data.Set(kBuildChannelKey, BuildChannel().name);

  return user_data;
}

}  // namespace ads::user_data
