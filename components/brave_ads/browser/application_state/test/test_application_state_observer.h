/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_TEST_TEST_APPLICATION_STATE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_TEST_TEST_APPLICATION_STATE_OBSERVER_H_

#include <cstddef>

#include "brave/components/brave_ads/browser/application_state/application_state_observer.h"

namespace brave_ads {

// Test implementation of `ApplicationStateObserver` that counts callback
// invocations.
class TestApplicationStateObserver : public ApplicationStateObserver {
 public:
  void OnBrowserDidBecomeActive() override;
  void OnBrowserDidResignActive() override;

  size_t foreground_count() const { return foreground_count_; }
  size_t background_count() const { return background_count_; }

 private:
  size_t foreground_count_ = 0;
  size_t background_count_ = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_APPLICATION_STATE_TEST_TEST_APPLICATION_STATE_OBSERVER_H_
