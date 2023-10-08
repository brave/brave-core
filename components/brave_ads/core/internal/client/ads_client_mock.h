/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CLIENT_ADS_CLIENT_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CLIENT_ADS_CLIENT_MOCK_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/client/ads_client.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep
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

  MOCK_METHOD(void, ShowReminder, (const mojom::ReminderType type));

  MOCK_METHOD(void, UpdateAdRewards, ());

  MOCK_METHOD(void,
              CacheAdEventForInstanceId,
              (const std::string& id,
               const std::string& type,
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
              GetBrowsingHistory,
              (const int max_count,
               const int recent_day_range,
               GetBrowsingHistoryCallback callback));

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
              LoadFileResource,
              (const std::string& id,
               const int version,
               LoadFileCallback callback));
  MOCK_METHOD(std::string, LoadDataResource, (const std::string& name));

  MOCK_METHOD(void,
              GetScheduledCaptcha,
              (const std::string& payment_id,
               GetScheduledCaptchaCallback callback));
  MOCK_METHOD(void,
              ShowScheduledCaptchaNotification,
              (const std::string& payment_id, const std::string& captcha_id));

  MOCK_METHOD(void,
              RunDBTransaction,
              (mojom::DBTransactionInfoPtr, RunDBTransactionCallback));

  MOCK_METHOD(void, RecordP2AEvents, (const std::vector<std::string>& events));

  MOCK_METHOD(void,
              AddFederatedLearningPredictorTrainingSample,
              (const std::vector<brave_federated::mojom::CovariateInfoPtr>
                   training_sample));

  MOCK_METHOD(bool, GetBooleanPref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetBooleanPref,
              (const std::string& path, const bool value));
  MOCK_METHOD(int, GetIntegerPref, (const std::string& path), (const));
  MOCK_METHOD(void, SetIntegerPref, (const std::string& path, const int value));
  MOCK_METHOD(double, GetDoublePref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetDoublePref,
              (const std::string& path, const double value));
  MOCK_METHOD(std::string, GetStringPref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetStringPref,
              (const std::string& path, const std::string& value));
  MOCK_METHOD(int64_t, GetInt64Pref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetInt64Pref,
              (const std::string& path, const int64_t value));
  MOCK_METHOD(uint64_t, GetUint64Pref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetUint64Pref,
              (const std::string& path, const uint64_t value));
  MOCK_METHOD(base::Time, GetTimePref, (const std::string& path), (const));
  MOCK_METHOD(void,
              SetTimePref,
              (const std::string& path, const base::Time value));
  MOCK_METHOD(absl::optional<base::Value::Dict>,
              GetDictPref,
              (const std::string& path),
              (const));
  MOCK_METHOD(void,
              SetDictPref,
              (const std::string& path, base::Value::Dict value));
  MOCK_METHOD(absl::optional<base::Value::List>,
              GetListPref,
              (const std::string& path),
              (const));
  MOCK_METHOD(void,
              SetListPref,
              (const std::string& path, base::Value::List value));
  MOCK_METHOD(void, ClearPref, (const std::string& path));
  MOCK_METHOD(bool, HasPrefPath, (const std::string& path), (const));
  MOCK_METHOD(absl::optional<base::Value>,
              GetLocalStatePref,
              (const std::string& path),
              (const));
  MOCK_METHOD(void,
              SetLocalStatePref,
              (const std::string& path, base::Value value));

  MOCK_METHOD(void,
              Log,
              (const char* file,
               const int line,
               const int verbose_level,
               const std::string& message));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CLIENT_ADS_CLIENT_MOCK_H_
