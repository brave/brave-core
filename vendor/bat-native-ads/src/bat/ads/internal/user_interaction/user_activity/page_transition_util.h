/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_

#include "absl/types/optional.h"
#include "bat/ads/internal/user_interaction/user_activity/user_activity_event_types.h"
#include "bat/ads/page_transition_types.h"

namespace ads {

bool IsNewNavigation(PageTransitionType type);

bool DidUseBackOrFowardButtonToTriggerNavigation(PageTransitionType type);

bool DidUseAddressBarToTriggerNavigation(PageTransitionType type);

bool DidNavigateToHomePage(PageTransitionType type);

bool DidTransitionFromExternalApplication(PageTransitionType type);

absl::optional<UserActivityEventType> ToUserActivityEventType(
    PageTransitionType type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_USER_ACTIVITY_PAGE_TRANSITION_UTIL_H_
