/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_OBSERVER_MOCK_H_

#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class BrowserManagerObserverMock : public BrowserManagerObserver {
 public:
  BrowserManagerObserverMock();

  BrowserManagerObserverMock(const BrowserManagerObserverMock&) = delete;
  BrowserManagerObserverMock& operator=(const BrowserManagerObserverMock&) =
      delete;

  ~BrowserManagerObserverMock() override;

  MOCK_METHOD(void, OnBrowserDidBecomeActive, ());

  MOCK_METHOD(void, OnBrowserDidResignActive, ());

  MOCK_METHOD(void, OnBrowserDidEnterForeground, ());

  MOCK_METHOD(void, OnBrowserDidEnterBackground, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_APPLICATION_STATE_BROWSER_MANAGER_OBSERVER_MOCK_H_
