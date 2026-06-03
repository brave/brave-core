/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/application_state/test/test_application_state_observer.h"

namespace brave_ads {

void TestApplicationStateObserver::OnBrowserDidBecomeActive() {
  ++foreground_count_;
}

void TestApplicationStateObserver::OnBrowserDidResignActive() {
  ++background_count_;
}

}  // namespace brave_ads
