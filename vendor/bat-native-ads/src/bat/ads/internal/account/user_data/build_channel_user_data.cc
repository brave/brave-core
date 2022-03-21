/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/build_channel_user_data.h"

#include "base/check.h"
#include "base/values.h"
#include "bat/ads/ads.h"

namespace ads {
namespace user_data {

base::DictionaryValue GetBuildChannel() {
  base::DictionaryValue user_data;

  DCHECK(!BuildChannel().name.empty());
  user_data.SetStringKey("buildChannel", BuildChannel().name);
  return user_data;
}

}  // namespace user_data
}  // namespace ads
