/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/application_state_observer.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/browser_list.h"
#endif

namespace ephemeral_storage {

ApplicationStateObserver::ApplicationStateObserver() {
#if BUILDFLAG(IS_ANDROID)
  app_status_listener_ = base::android::ApplicationStatusListener::New(
      base::BindRepeating(&ApplicationStateObserver::OnApplicationStateChange,
                          weak_ptr_factory_.GetWeakPtr()));
#else
  BrowserList::AddObserver(this);
#endif
}

ApplicationStateObserver::~ApplicationStateObserver() {
LOG(INFO) << "[SHRED] ApplicationStateObserver unbinded";
#if BUILDFLAG(IS_ANDROID)
  app_status_listener_.reset();
#else
  BrowserList::RemoveObserver(this);
#endif
}

void ApplicationStateObserver::AddObserver(Observer* observer) {
  observers_.push_back(observer);
}

void ApplicationStateObserver::RemoveObserver(Observer* observer) {
  auto it = std::find(observers_.begin(), observers_.end(), observer);
  if (it != observers_.end()) {
    observers_.erase(it);
  }
}

#if BUILDFLAG(IS_ANDROID)
void ApplicationStateObserver::TriggerCurrentStateNotification() {
  // Call OnApplicationStateChange once to handle current state
  OnApplicationStateChange(
      base::android::ApplicationStatusListener::GetState());
  LOG(INFO) << "[SHRED] ApplicationStateObserver binded last_state: "
            << last_state_;
}

void ApplicationStateObserver::OnApplicationStateChange(
    base::android::ApplicationState state) {
LOG(INFO) << "[SHRED] Application state changed: " << state;
  if (state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    LOG(INFO) << "[SHRED] Application state changed #100";
    if (!has_notified_active_) {
        LOG(INFO) << "[SHRED] Application state changed #200";
      NotifyApplicationBecameActive();
      has_notified_active_ = true;
    }
  } else if (last_state_ != state &&
             last_state_ ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    LOG(INFO) << "[SHRED] Application state changed #300";
    NotifyApplicationBecameInactive();
    has_notified_active_ = false;
  }

  LOG(INFO) << "[SHRED] Application state changed #400";
  last_state_ = state;
}
#endif

#if !BUILDFLAG(IS_ANDROID)
void ApplicationStateObserver::OnBrowserAdded(Browser* browser) {
  if (!has_notified_active_) {
    NotifyApplicationBecameActive();
    has_notified_active_ = true;
  }
}
#endif

void ApplicationStateObserver::NotifyApplicationBecameActive() {
  for (Observer* observer : observers_) {
    observer->OnApplicationBecameActive();
  }
}

void ApplicationStateObserver::NotifyApplicationBecameInactive() {
  for (Observer* observer : observers_) {
    observer->OnApplicationBecameInactive();
  }
}

}  // namespace ephemeral_storage
