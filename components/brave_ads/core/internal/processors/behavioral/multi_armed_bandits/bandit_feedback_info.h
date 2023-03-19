/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_MULTI_ARMED_BANDITS_BANDIT_FEEDBACK_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_MULTI_ARMED_BANDITS_BANDIT_FEEDBACK_INFO_H_

#include <string>

#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"

namespace brave_ads::processor {

struct BanditFeedbackInfo final {
  std::string segment;
  mojom::NotificationAdEventType ad_event_type;
};

}  // namespace brave_ads::processor

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_MULTI_ARMED_BANDITS_BANDIT_FEEDBACK_INFO_H_
