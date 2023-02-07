/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_

#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "bat/ads/public/interfaces/ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom.h"  // IWYU pragma: keep
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

class AdsClientMock : public AdsClient {
 public:
  AdsClientMock();

  AdsClientMock(const AdsClientMock& other) = delete;
  AdsClientMock& operator=(const AdsClientMock& other) = delete;

  AdsClientMock(AdsClientMock&& other) noexcept = delete;
  AdsClientMock& operator=(AdsClientMock&& other) noexcept = delete;

  ~AdsClientMock() override;

  MOCK_CONST_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD0(IsBrowserActive, bool());
  MOCK_CONST_METHOD0(IsBrowserInFullScreenMode, bool());

  MOCK_METHOD0(CanShowNotificationAds, bool());
  MOCK_CONST_METHOD0(CanShowNotificationAdsWhileBrowserIsBackgrounded, bool());
  MOCK_METHOD1(ShowNotificationAd, void(const NotificationAdInfo& ad));
  MOCK_METHOD1(CloseNotificationAd, void(const std::string& placement_id));

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
                    const int days_ago,
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
  MOCK_METHOD0(ClearScheduledCaptcha, void());

  MOCK_METHOD2(RunDBTransaction,
               void(mojom::DBTransactionInfoPtr, RunDBTransactionCallback));

  MOCK_METHOD2(RecordP2AEvent,
               void(const std::string& name, base::Value::List value));

  MOCK_METHOD1(LogTrainingInstance,
               void(const std::vector<brave_federated::mojom::CovariateInfoPtr>
                        training_instance));

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

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
