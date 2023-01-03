/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_OBSERVER_H_

#include "base/observer_list_types.h"
#include "base/time/time.h"

namespace ads {

class IdleDetectionManagerObserver : public base::CheckedObserver {
 public:
  // Invoked when the user becomes active. |idle_time| is the amount of time in
  // seconds that the user was idle. |screen_was_locked| is |true| if the user's
  // screen was locked, otherwise |false|.
  virtual void OnUserDidBecomeActive(const base::TimeDelta idle_time,
                                     const bool screen_was_locked) {}

  // Invoked when the user becomes idle.
  virtual void OnUserDidBecomeIdle() {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_OBSERVER_H_
