/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_SCORING_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_SCORING_H_

#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_event_info.h"
#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_trigger_info.h"

namespace ads {

double GetUserActivityScore(const UserActivityTriggerList& triggers,
                            const UserActivityEventList& events);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_USER_ACTIVITY_USER_ACTIVITY_SCORING_H_
