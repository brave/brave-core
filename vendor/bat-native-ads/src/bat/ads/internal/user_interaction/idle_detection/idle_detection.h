/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_H_

#include "bat/ads/ads_client_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

class IdleDetection : public AdsClientObserver {
 public:
  IdleDetection();

  IdleDetection(const IdleDetection& other) = delete;
  IdleDetection& operator=(const IdleDetection& other) = delete;

  IdleDetection(IdleDetection&& other) noexcept = delete;
  IdleDetection& operator=(IdleDetection&& other) noexcept = delete;

  ~IdleDetection() override;

 private:
  // AdsClientObserver:
  void OnUserDidBecomeActive(base::TimeDelta idle_time,
                             bool screen_was_locked) override;
  void OnUserDidBecomeIdle() override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_H_
