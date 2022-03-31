/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_FEATURES_H_

#include <string>

#include "base/feature_list.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {
namespace features {
namespace user_activity {

extern const base::Feature kFeature;

bool IsEnabled();

std::string GetTriggers();
base::TimeDelta GetTimeWindow();
double GetThreshold();

base::TimeDelta GetIdleTimeThreshold();
base::TimeDelta GetMaximumIdleTime();

bool ShouldDetectWasLocked();

}  // namespace user_activity
}  // namespace features
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_ACTIVITY_USER_ACTIVITY_FEATURES_H_
