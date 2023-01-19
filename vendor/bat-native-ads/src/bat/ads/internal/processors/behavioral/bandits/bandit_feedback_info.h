/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_BANDIT_FEEDBACK_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_BANDIT_FEEDBACK_INFO_H_

#include <string>

#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads::processor {

struct BanditFeedbackInfo final {
  std::string segment;
  mojom::NotificationAdEventType ad_event_type;
};

}  // namespace ads::processor

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_BANDIT_FEEDBACK_INFO_H_
