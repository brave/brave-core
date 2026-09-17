/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/application_state_observer.h"

#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/logging.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/profile_browser_collection.h"
#endif

namespace ephemeral_storage {

#if BUILDFLAG(IS_ANDROID)
ApplicationStateObserver::ApplicationStateObserver() = default;
#else
ApplicationStateObserver::ApplicationStateObserver(
    content::BrowserContext* context)
    : context_(context) {
  browser_collection_observation_.Observe(
      ProfileBrowserCollection::GetForProfile(
          Profile::FromBrowserContext(context)));
}
#endif

ApplicationStateObserver::~ApplicationStateObserver() = default;

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
  // On Android, all work should be handled when the application launches, so we
  // must go through the app state transition from inactive to active
  NotifyApplicationBecameInactive();
  NotifyApplicationBecameActive();
}
#endif

#if !BUILDFLAG(IS_ANDROID)
void ApplicationStateObserver::OnBrowserClosed(BrowserWindowInterface* browser) {
  if (browser->GetProfile() != Profile::FromBrowserContext(context_)) {
LOG(INFO) << "[SHRED] ApplicationStateObserver::OnBrowserClosed Wrong profile";
    return;
  }

  if (!browser_collection_observation_.GetSource()->IsEmpty()) {
LOG(INFO) << "[SHRED] ApplicationStateObserver::OnBrowserClosed Not the last Browser window";
    return;
  }

LOG(INFO) << "[SHRED] ApplicationStateObserver::OnBrowserClosed Initiate Cleanup #100";
  browser_collection_observation_.Reset();
  NotifyApplicationBecameInactive();
LOG(INFO) << "[SHRED] ApplicationStateObserver::OnBrowserClosed Initiate Cleanup #200";
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
