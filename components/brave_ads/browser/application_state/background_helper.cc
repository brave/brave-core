/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/application_state/background_helper.h"

#include "brave/components/brave_ads/browser/application_state/background_helper_observer.h"

namespace brave_ads {

BackgroundHelper::BackgroundHelper() = default;

BackgroundHelper::~BackgroundHelper() = default;

void BackgroundHelper::AddObserver(BackgroundHelperObserver* const observer) {
  observers_.AddObserver(observer);
}

void BackgroundHelper::RemoveObserver(
    BackgroundHelperObserver* const observer) {
  observers_.RemoveObserver(observer);
}

bool BackgroundHelper::IsInForeground() const {
  return true;
}

void BackgroundHelper::NotifyDidEnterForeground() {
  observers_.Notify(&BackgroundHelperObserver::OnBrowserDidEnterForeground);
}

void BackgroundHelper::NotifyDidEnterBackground() {
  observers_.Notify(&BackgroundHelperObserver::OnBrowserDidEnterBackground);
}

}  // namespace brave_ads
