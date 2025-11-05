/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsClientMock : public AdsClient {
 public:
  AdsClientMock();

  AdsClientMock(const AdsClientMock&) = delete;
  AdsClientMock& operator=(const AdsClientMock&) = delete;

  ~AdsClientMock() override;

  MOCK_METHOD(void, AddObserver, (AdsClientNotifierObserver*));
  MOCK_METHOD(void, RemoveObserver, (AdsClientNotifierObserver*));
  MOCK_METHOD(void, NotifyPendingObservers, ());

  MOCK_METHOD(bool, IsNetworkConnectionAvailable, (), (const));

  MOCK_METHOD(bool, IsBrowserActive, (), (const));
  MOCK_METHOD(bool, IsBrowserInFullScreenMode, (), (const));

  MOCK_METHOD(bool, CanShowNotificationAds, (), (const));
  MOCK_METHOD(bool,
              CanShowNotificationAdsWhileBrowserIsBackgrounded,
              (),
              (const));
  MOCK_METHOD(void, ShowNotificationAd, (const NotificationAdInfo&));
  MOCK_METHOD(void, CloseNotificationAd, (const std::string&));

  MOCK_METHOD(void, GetSiteHistory, (int, int, GetSiteHistoryCallback));

  MOCK_METHOD(void, UrlRequest, (mojom::UrlRequestInfoPtr, UrlRequestCallback));

  MOCK_METHOD(void,
              Save,
              (const std::string&, const std::string&, SaveCallback));
  MOCK_METHOD(void, Load, (const std::string&, LoadCallback));

  MOCK_METHOD(void,
              LoadResourceComponent,
              (const std::string&, int, LoadFileCallback));

  MOCK_METHOD(void,
              ShowScheduledCaptcha,
              (const std::string&, const std::string&));

  MOCK_METHOD(bool, FindProfilePref, (const std::string&), (const));
  MOCK_METHOD(std::optional<base::Value>, GetProfilePref, (const std::string&));
  MOCK_METHOD(void, SetProfilePref, (const std::string&, base::Value));
  MOCK_METHOD(void, ClearProfilePref, (const std::string&));
  MOCK_METHOD(bool, HasProfilePrefPath, (const std::string&), (const));

  MOCK_METHOD(bool, FindLocalStatePref, (const std::string&), (const));
  MOCK_METHOD(std::optional<base::Value>,
              GetLocalStatePref,
              (const std::string&));
  MOCK_METHOD(void, SetLocalStatePref, (const std::string&, base::Value));
  MOCK_METHOD(void, ClearLocalStatePref, (const std::string&));
  MOCK_METHOD(bool, HasLocalStatePrefPath, (const std::string&), (const));

  MOCK_METHOD(base::Value::Dict, GetVirtualPrefs, (), (const));

  MOCK_METHOD(void, Log, (const char*, int, int, const std::string&));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_
