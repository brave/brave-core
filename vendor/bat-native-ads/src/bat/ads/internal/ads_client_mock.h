/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_

#include "bat/ads/ads_client.h"

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class AdsClientMock : public AdsClient {
 public:
  AdsClientMock();

  ~AdsClientMock() override;

  MOCK_CONST_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD0(IsForeground, bool());

  MOCK_CONST_METHOD0(IsFullScreen, bool());

  MOCK_METHOD0(ShouldShowNotifications, bool());

  MOCK_CONST_METHOD0(CanShowBackgroundNotifications, bool());

  MOCK_METHOD1(ShowNotification,
               void(const AdNotificationInfo& ad_notification));

  MOCK_METHOD1(CloseNotification, void(const std::string& uuid));

  MOCK_CONST_METHOD3(RecordAdEvent,
                     void(const std::string& type,
                          const std::string& confirmation_type,
                          const uint64_t timestamp));

  MOCK_CONST_METHOD2(
      GetAdEvents,
      std::vector<uint64_t>(const std::string& ad_type,
                            const std::string& confirmation_type));

  MOCK_METHOD2(UrlRequest,
               void(UrlRequestPtr url_request, UrlRequestCallback callback));

  MOCK_METHOD3(Save,
               void(const std::string& name,
                    const std::string& value,
                    ResultCallback callback));

  MOCK_METHOD2(Load, void(const std::string& name, LoadCallback callback));

  MOCK_METHOD3(LoadAdsResource,
               void(const std::string& id,
                    const int version,
                    LoadCallback callback));

  MOCK_METHOD3(GetBrowsingHistory,
               void(const int max_count,
                    const int days_ago,
                    GetBrowsingHistoryCallback callback));

  MOCK_METHOD1(LoadResourceForId, std::string(const std::string& id));

  MOCK_METHOD2(RunDBTransaction,
               void(DBTransactionPtr, RunDBTransactionCallback));

  MOCK_METHOD0(OnAdRewardsChanged, void());

  MOCK_METHOD3(RecordP2AEvent,
               void(const std::string& name,
                    const ads::P2AEventType type,
                    const std::string& value));

  MOCK_METHOD4(Log,
               void(const char* file,
                    const int line,
                    const int verbose_level,
                    const std::string& message));

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

  MOCK_METHOD1(ClearPref, void(const std::string& path));
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
