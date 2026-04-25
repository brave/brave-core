/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_

#include <optional>

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_event_types.h"
#include "ui/base/page_transition_types.h"

namespace brave_ads {

std::optional<UserActivityEventType> ToUserActivityEventType(
    ui::PageTransition page_transition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_
