/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/notification_ad_unittest_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

namespace ads {

bool IsServingAdAtRegularIntervals() {
  const base::Time serve_ad_at =
      ClientStateManager::GetInstance()->GetServeAdAt();
  return !serve_ad_at.is_null();
}

}  // namespace ads
