/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_

#include "base/observer_list.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager_observer.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace ads {

class IdleDetectionManager final {
 public:
  IdleDetectionManager();
  ~IdleDetectionManager();

  IdleDetectionManager(const IdleDetectionManager&) = delete;
  IdleDetectionManager& operator=(const IdleDetectionManager&) = delete;

  static IdleDetectionManager* GetInstance();

  static bool HasInstance();

  void AddObserver(IdleDetectionManagerObserver* observer);
  void RemoveObserver(IdleDetectionManagerObserver* observer);

  void UserDidBecomeActive(const base::TimeDelta idle_time,
                           const bool was_locked) const;
  void UserDidBecomeIdle() const;

 private:
  void NotifyUserDidBecomeActive(const base::TimeDelta idle_time,
                                 const bool was_locked) const;
  void NotifyUserDidBecomeIdle() const;

  base::ObserverList<IdleDetectionManagerObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_USER_INTERACTION_IDLE_DETECTION_IDLE_DETECTION_MANAGER_H_
