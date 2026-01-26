/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/application_state_observer.h"

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/logging.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#else
#include "brave/browser/brave_shields/android/brave_first_party_storage_cleaner_utils.h"
#endif

namespace ephemeral_storage {

#if BUILDFLAG(IS_ANDROID)
ApplicationStateObserver::ApplicationStateObserver() {
  app_status_listener_ = base::android::ApplicationStatusListener::New(
      base::BindRepeating(&ApplicationStateObserver::OnApplicationStateChange,
                          weak_ptr_factory_.GetWeakPtr()));
}
#else
ApplicationStateObserver::ApplicationStateObserver(
    content::BrowserContext* context)
    : context_(context) {
  BrowserList::AddObserver(this);
}
#endif

ApplicationStateObserver::~ApplicationStateObserver() {
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
void ApplicationStateObserver::TriggerCurrentAppStateNotification() {
  // Call OnApplicationStateChange once to handle current state
  OnApplicationStateChange(
      base::android::ApplicationStatusListener::GetState());
}

void ApplicationStateObserver::OnApplicationStateChange(
    base::android::ApplicationState new_state) {
  auto app_in_stack = brave_shields::IsAppInTaskStack();
  if (new_state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    if (!has_notified_active_) {
      NotifyApplicationBecameActive();
      has_notified_active_ = true;
    }
  } else if (!app_in_stack && current_state_ != new_state &&
             current_state_ ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES) {
    NotifyApplicationBecameInactive();
    has_notified_active_ = false;
  }
  current_state_ = new_state;
}
#endif

#if !BUILDFLAG(IS_ANDROID)
void ApplicationStateObserver::OnBrowserAdded(Browser* browser) {
  if (browser->profile() != Profile::FromBrowserContext(context_)) {
    return;
  }

  if (!has_notified_active_) {
    has_notified_active_ = true;

    // No need to observe anymore.
    BrowserList::RemoveObserver(this);

    // Trigger the callback notifications after a cycle of the main loop to
    // handle all windows
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&ApplicationStateObserver::NotifyApplicationBecameActive,
                       weak_ptr_factory_.GetWeakPtr()));
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
