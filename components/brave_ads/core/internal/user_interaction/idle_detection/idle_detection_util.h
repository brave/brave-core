/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_UTIL_H_

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

bool MaybeScreenWasLocked(bool screen_was_locked);

bool HasExceededMaximumIdleTime(base::TimeDelta idle_time);

bool MaybeUpdateIdleTimeThreshold();

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_UTIL_H_
