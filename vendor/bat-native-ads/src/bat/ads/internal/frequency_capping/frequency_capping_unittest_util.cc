/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

AdHistory GenerateAdHistory(
    const CreativeAdInfo& ad,
    const ConfirmationType& confirmation_type) {
  AdHistory history;

  history.uuid = base::GenerateGUID();
  history.ad_content.creative_instance_id = ad.creative_instance_id;
  history.ad_content.creative_set_id = ad.creative_set_id;
  history.ad_content.campaign_id = ad.campaign_id;
  history.ad_content.ad_action = confirmation_type;
  history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  return history;
}

}  // namespace ads
