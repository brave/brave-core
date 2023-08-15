/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
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

  MOCK_METHOD0(OnNotifyDidInitializeAds, void());
  MOCK_METHOD1(OnNotifyLocaleDidChange, void(const std::string&));
  MOCK_METHOD1(OnNotifyPrefDidChange, void(const std::string&));
  MOCK_METHOD2(OnNotifyDidUpdateResourceComponent,
               void(const std::string&, const std::string&));
  MOCK_METHOD2(OnNotifyRewardsWalletDidUpdate,
               void(const std::string&, const std::string&));
  MOCK_METHOD3(OnNotifyTabTextContentDidChange,
               void(int32_t, const std::vector<GURL>&, const std::string&));
  MOCK_METHOD3(OnNotifyTabHtmlContentDidChange,
               void(int32_t, const std::vector<GURL>&, const std::string&));
  MOCK_METHOD1(OnNotifyTabDidStartPlayingMedia, void(int32_t));
  MOCK_METHOD1(OnNotifyTabDidStopPlayingMedia, void(int32_t));
  MOCK_METHOD3(OnNotifyTabDidChange,
               void(int32_t, const std::vector<GURL>&, bool));
  MOCK_METHOD1(OnNotifyDidCloseTab, void(int32_t));
  MOCK_METHOD1(OnNotifyUserGestureEventTriggered, void(int32_t));
  MOCK_METHOD0(OnNotifyUserDidBecomeIdle, void());
  MOCK_METHOD2(OnNotifyUserDidBecomeActive, void(base::TimeDelta, bool));
  MOCK_METHOD0(OnNotifyBrowserDidEnterForeground, void());
  MOCK_METHOD0(OnNotifyBrowserDidEnterBackground, void());
  MOCK_METHOD0(OnNotifyBrowserDidBecomeActive, void());
  MOCK_METHOD0(OnNotifyBrowserDidResignActive, void());
  MOCK_METHOD0(OnNotifyDidSolveAdaptiveCaptcha, void());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_NOTIFIER_OBSERVER_MOCK_H_
