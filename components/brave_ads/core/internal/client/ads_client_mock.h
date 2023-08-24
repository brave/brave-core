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

  MOCK_METHOD1(AddObserver, void(AdsClientNotifierObserver*));
  MOCK_METHOD1(RemoveObserver, void(AdsClientNotifierObserver*));
  MOCK_METHOD0(NotifyPendingObservers, void());

  MOCK_CONST_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD0(IsBrowserActive, bool());
  MOCK_CONST_METHOD0(IsBrowserInFullScreenMode, bool());

  MOCK_METHOD0(CanShowNotificationAds, bool());
  MOCK_CONST_METHOD0(CanShowNotificationAdsWhileBrowserIsBackgrounded, bool());
  MOCK_METHOD1(ShowNotificationAd, void(const NotificationAdInfo& ad));
  MOCK_METHOD1(CloseNotificationAd, void(const std::string& placement_id));

  MOCK_METHOD1(ShowReminder, void(const mojom::ReminderType type));

  MOCK_METHOD0(UpdateAdRewards, void());

  MOCK_CONST_METHOD4(RecordAdEventForId,
                     void(const std::string& id,
                          const std::string& type,
                          const std::string& confirmation_type,
                          const base::Time time));
  MOCK_CONST_METHOD2(
      GetAdEventHistory,
      std::vector<base::Time>(const std::string& ad_type,
                              const std::string& confirmation_type));
  MOCK_CONST_METHOD1(ResetAdEventHistoryForId, void(const std::string& id));

  MOCK_METHOD3(GetBrowsingHistory,
               void(const int max_count,
                    const int recent_day_range,
                    GetBrowsingHistoryCallback callback));

  MOCK_METHOD2(UrlRequest,
               void(mojom::UrlRequestInfoPtr url_request,
                    UrlRequestCallback callback));

  MOCK_METHOD3(Save,
               void(const std::string& name,
                    const std::string& value,
                    SaveCallback callback));
  MOCK_METHOD2(Load, void(const std::string& name, LoadCallback callback));
  MOCK_METHOD3(LoadFileResource,
               void(const std::string& id,
                    const int version,
                    LoadFileCallback callback));
  MOCK_METHOD1(LoadDataResource, std::string(const std::string& name));

  MOCK_METHOD2(GetScheduledCaptcha,
               void(const std::string& payment_id,
                    GetScheduledCaptchaCallback callback));
  MOCK_METHOD2(ShowScheduledCaptchaNotification,
               void(const std::string& payment_id,
                    const std::string& captcha_id));

  MOCK_METHOD2(RunDBTransaction,
               void(mojom::DBTransactionInfoPtr, RunDBTransactionCallback));

  MOCK_METHOD1(RecordP2AEvents, void(base::Value::List events));

  MOCK_METHOD1(AddTrainingSample,
               void(const std::vector<brave_federated::mojom::CovariateInfoPtr>
                        training_sample));

  MOCK_CONST_METHOD1(GetBooleanPref, bool(const std::string& path));
  MOCK_METHOD2(SetBooleanPref, void(const std::string& path, const bool value));
  MOCK_CONST_METHOD1(GetIntegerPref, int(const std::string& path));
  MOCK_METHOD2(SetIntegerPref, void(const std::string& path, const int value));
  MOCK_CONST_METHOD1(GetDoublePref, double(const std::string& path));
  MOCK_METHOD2(SetDoublePref,
               void(const std::string& path, const double value));
  MOCK_CONST_METHOD1(GetStringPref, std::string(const std::string& path));
  MOCK_METHOD2(SetStringPref,
               void(const std::string& path, const std::string& value));
  MOCK_CONST_METHOD1(GetInt64Pref, int64_t(const std::string& path));
  MOCK_METHOD2(SetInt64Pref,
               void(const std::string& path, const int64_t value));
  MOCK_CONST_METHOD1(GetUint64Pref, uint64_t(const std::string& path));
  MOCK_METHOD2(SetUint64Pref,
               void(const std::string& path, const uint64_t value));
  MOCK_CONST_METHOD1(GetTimePref, base::Time(const std::string& path));
  MOCK_METHOD2(SetTimePref,
               void(const std::string& path, const base::Time value));
  MOCK_CONST_METHOD1(
      GetDictPref,
      absl::optional<base::Value::Dict>(const std::string& path));
  MOCK_METHOD2(SetDictPref,
               void(const std::string& path, base::Value::Dict value));
  MOCK_CONST_METHOD1(
      GetListPref,
      absl::optional<base::Value::List>(const std::string& path));
  MOCK_METHOD2(SetListPref,
               void(const std::string& path, base::Value::List value));
  MOCK_METHOD1(ClearPref, void(const std::string& path));
  MOCK_CONST_METHOD1(HasPrefPath, bool(const std::string& path));

  MOCK_METHOD4(Log,
               void(const char* file,
                    const int line,
                    const int verbose_level,
                    const std::string& message));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CLIENT_ADS_CLIENT_MOCK_H_
