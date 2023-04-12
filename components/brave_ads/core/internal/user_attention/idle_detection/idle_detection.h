/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_H_

#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

class IdleDetection : public AdsClientNotifierObserver {
 public:
  IdleDetection();

  IdleDetection(const IdleDetection&) = delete;
  IdleDetection& operator=(const IdleDetection&) = delete;

  IdleDetection(IdleDetection&&) noexcept = delete;
  IdleDetection& operator=(IdleDetection&&) noexcept = delete;

  ~IdleDetection() override;

 private:
  // AdsClientNotifierObserver:
  void OnNotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                   bool screen_was_locked) override;
  void OnNotifyUserDidBecomeIdle() override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_H_
