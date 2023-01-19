/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/notification_ad_info.h"

namespace ads {

bool NotificationAdInfo::IsValid() const {
  if (!AdInfo::IsValid()) {
    return false;
  }

  if (title.empty() || body.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
