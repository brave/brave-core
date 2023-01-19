/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"

#include "base/time/time.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"
#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_util.h"

namespace ads {

namespace {
IdleDetectionManager* g_idle_detection_manager_instance = nullptr;
}  // namespace

IdleDetectionManager::IdleDetectionManager() {
  DCHECK(!g_idle_detection_manager_instance);
  g_idle_detection_manager_instance = this;

  MaybeUpdateIdleTimeThreshold();
}

IdleDetectionManager::~IdleDetectionManager() {
  DCHECK_EQ(this, g_idle_detection_manager_instance);
  g_idle_detection_manager_instance = nullptr;
}

// static
IdleDetectionManager* IdleDetectionManager::GetInstance() {
  DCHECK(g_idle_detection_manager_instance);
  return g_idle_detection_manager_instance;
}

// static
bool IdleDetectionManager::HasInstance() {
  return !!g_idle_detection_manager_instance;
}

void IdleDetectionManager::AddObserver(IdleDetectionManagerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void IdleDetectionManager::RemoveObserver(
    IdleDetectionManagerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void IdleDetectionManager::UserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) const {
  BLOG(1, "User is active after " << idle_time);

  MaybeUpdateIdleTimeThreshold();

  SetLastUnIdleTimeDiagnosticEntry();

  NotifyUserDidBecomeActive(idle_time, screen_was_locked);
}

void IdleDetectionManager::UserDidBecomeIdle() const {
  BLOG(1, "User is idle");

  NotifyUserDidBecomeIdle();
}

///////////////////////////////////////////////////////////////////////////////

void IdleDetectionManager::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) const {
  for (IdleDetectionManagerObserver& observer : observers_) {
    observer.OnUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void IdleDetectionManager::NotifyUserDidBecomeIdle() const {
  for (IdleDetectionManagerObserver& observer : observers_) {
    observer.OnUserDidBecomeIdle();
  }
}

}  // namespace ads
