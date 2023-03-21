/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_

#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/user_attention/idle_detection/idle_detection_manager_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

class IdleDetectionManager final {
 public:
  IdleDetectionManager();

  IdleDetectionManager(const IdleDetectionManager& other) = delete;
  IdleDetectionManager& operator=(const IdleDetectionManager& other) = delete;

  IdleDetectionManager(IdleDetectionManager&& other) noexcept = delete;
  IdleDetectionManager& operator=(IdleDetectionManager&& other) noexcept =
      delete;

  ~IdleDetectionManager();

  static IdleDetectionManager* GetInstance();

  static bool HasInstance();

  void AddObserver(IdleDetectionManagerObserver* observer);
  void RemoveObserver(IdleDetectionManagerObserver* observer);

  void UserDidBecomeActive(base::TimeDelta idle_time,
                           bool screen_was_locked) const;
  void UserDidBecomeIdle() const;

 private:
  void NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                 bool screen_was_locked) const;
  void NotifyUserDidBecomeIdle() const;

  base::ObserverList<IdleDetectionManagerObserver> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_
