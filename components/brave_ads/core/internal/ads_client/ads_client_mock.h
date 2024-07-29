/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class AdsClientMock : public AdsClient {
 public:
  AdsClientMock();

  AdsClientMock(const AdsClientMock&) = delete;
  AdsClientMock& operator=(const AdsClientMock&) = delete;

  AdsClientMock(AdsClientMock&&) noexcept = delete;
  AdsClientMock& operator=(AdsClientMock&&) noexcept = delete;

  ~AdsClientMock() override;

  MOCK_METHOD(void, AddObserver, (AdsClientNotifierObserver*));
  MOCK_METHOD(void, RemoveObserver, (AdsClientNotifierObserver*));
  MOCK_METHOD(void, NotifyPendingObservers, ());

  MOCK_METHOD(bool, IsNetworkConnectionAvailable, (), (const));

  MOCK_METHOD(bool, IsBrowserActive, (), (const));
  MOCK_METHOD(bool, IsBrowserInFullScreenMode, (), (const));

  MOCK_METHOD(bool, CanShowNotificationAds, ());
  MOCK_METHOD(bool,
              CanShowNotificationAdsWhileBrowserIsBackgrounded,
              (),
              (const));
  MOCK_METHOD(void, ShowNotificationAd, (const NotificationAdInfo& ad));
  MOCK_METHOD(void, CloseNotificationAd, (const std::string& placement_id));

  MOCK_METHOD(void,
              CacheAdEventForInstanceId,
              (const std::string& id,
               const std::string& ad_type,
               const std::string& confirmation_type,
               const base::Time time),
              (const));
  MOCK_METHOD(std::vector<base::Time>,
              GetCachedAdEvents,
              (const std::string& ad_type,
               const std::string& confirmation_type),
              (const));
  MOCK_METHOD(void,
              ResetAdEventCacheForInstanceId,
              (const std::string& id),
              (const));

  MOCK_METHOD(void,
              GetSiteHistory,
              (const int max_count,
               const int recent_day_range,
               GetSiteHistoryCallback callback));

  MOCK_METHOD(void,
              UrlRequest,
              (mojom::UrlRequestInfoPtr url_request,
               UrlRequestCallback callback));

  MOCK_METHOD(void,
              Save,
              (const std::string& name,
               const std::string& value,
               SaveCallback callback));
  MOCK_METHOD(void, Load, (const std::string& name, LoadCallback callback));
  MOCK_METHOD(void,
              LoadResourceComponent,
              (const std::string& id,
               const int version,
               LoadFileCallback callback));
  MOCK_METHOD(std::string, LoadDataResource, (const std::string& name));

  MOCK_METHOD(void,
              ShowScheduledCaptcha,
              (const std::string& payment_id, const std::string& captcha_id));

  MOCK_METHOD(void,
              RunDBTransaction,
              (mojom::DBTransactionInfoPtr, RunDBTransactionCallback));

  MOCK_METHOD(void, RecordP2AEvents, (const std::vector<std::string>& events));

  MOCK_METHOD(std::optional<base::Value>,
              GetProfilePref,
              (const std::string& path));
  MOCK_METHOD(void,
              SetProfilePref,
              (const std::string& path, base::Value value));
  MOCK_METHOD(void, ClearProfilePref, (const std::string& path));
  MOCK_METHOD(bool, HasProfilePrefPath, (const std::string& path), (const));

  MOCK_METHOD(std::optional<base::Value>,
              GetLocalStatePref,
              (const std::string& path));
  MOCK_METHOD(void,
              SetLocalStatePref,
              (const std::string& path, base::Value value));
  MOCK_METHOD(void, ClearLocalStatePref, (const std::string& path));
  MOCK_METHOD(bool, HasLocalStatePrefPath, (const std::string& path), (const));

  MOCK_METHOD(void,
              Log,
              (const char* file,
               const int line,
               const int verbose_level,
               const std::string& message));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CLIENT_ADS_CLIENT_MOCK_H_
