/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/background_helper.h"

namespace brave_ads {

BackgroundHelper::BackgroundHelper() {}

BackgroundHelper::~BackgroundHelper() {}

bool BackgroundHelper::IsForeground() const {
  return true;
}

void BackgroundHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BackgroundHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BackgroundHelper::TriggerOnBackground() {
  for (auto& observer : observers_) {
    observer.OnBackground();
  }
}

void BackgroundHelper::TriggerOnForeground() {
  for (auto& observer : observers_) {
    observer.OnForeground();
  }
}

#if !defined(OS_MAC) && !defined(OS_WIN) && !defined(OS_LINUX) && !defined(OS_ANDROID)  // NOLINT
BackgroundHelper* BackgroundHelper::GetInstance() {
  // just return a dummy background helper for all other platforms
  return base::Singleton<BackgroundHelper>::get();
}
#endif

}  // namespace brave_ads
