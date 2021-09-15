/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_UTIL_H_

#include <string>

#include "bat/ads/internal/user_activity/user_activity_trigger_info_aliases.h"

namespace ads {

UserActivityTriggerList ToUserActivityTriggers(const std::string& param_value);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_UTIL_H_
