/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_

#include <cstdint>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsClientNotifierObserverMock : public AdsClientNotifierObserver {
 public:
  AdsClientNotifierObserverMock();

  AdsClientNotifierObserverMock(const AdsClientNotifierObserverMock&) = delete;
  AdsClientNotifierObserverMock& operator=(
      const AdsClientNotifierObserverMock&) = delete;

  AdsClientNotifierObserverMock(AdsClientNotifierObserverMock&&) noexcept =
      delete;
  AdsClientNotifierObserverMock& operator=(
      AdsClientNotifierObserverMock&&) noexcept = delete;

  ~AdsClientNotifierObserverMock() override;

  MOCK_METHOD(void, OnNotifyDidInitializeAds, ());
  MOCK_METHOD(void, OnNotifyLocaleDidChange, (const std::string&));
  MOCK_METHOD(void, OnNotifyPrefDidChange, (const std::string&));
  MOCK_METHOD(void,
              OnNotifyResourceComponentDidChange,
              (const std::string&, const std::string&));
  MOCK_METHOD(void,
              OnNotifyDidUnregisterResourceComponent,
              (const std::string&));
  MOCK_METHOD(void,
              OnNotifyRewardsWalletDidUpdate,
              (const std::string&, const std::string&));
  MOCK_METHOD(void,
              OnNotifyTabTextContentDidChange,
              (int32_t, const std::vector<GURL>&, const std::string&));
  MOCK_METHOD(void,
              OnNotifyTabHtmlContentDidChange,
              (int32_t, const std::vector<GURL>&, const std::string&));
  MOCK_METHOD(void, OnNotifyTabDidStartPlayingMedia, (int32_t));
  MOCK_METHOD(void, OnNotifyTabDidStopPlayingMedia, (int32_t));
  MOCK_METHOD(void,
              OnNotifyTabDidChange,
              (int32_t, const std::vector<GURL>&, bool, bool, bool, bool));
  MOCK_METHOD(void, OnNotifyDidCloseTab, (int32_t));
  MOCK_METHOD(void, OnNotifyUserGestureEventTriggered, (int32_t));
  MOCK_METHOD(void, OnNotifyUserDidBecomeIdle, ());
  MOCK_METHOD(void, OnNotifyUserDidBecomeActive, (base::TimeDelta, bool));
  MOCK_METHOD(void, OnNotifyBrowserDidEnterForeground, ());
  MOCK_METHOD(void, OnNotifyBrowserDidEnterBackground, ());
  MOCK_METHOD(void, OnNotifyBrowserDidBecomeActive, ());
  MOCK_METHOD(void, OnNotifyBrowserDidResignActive, ());
  MOCK_METHOD(void, OnNotifyDidSolveAdaptiveCaptcha, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_
