/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
#define BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_

#include "bat/ads/ads_client.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class AdsClientMock : public AdsClient {
 public:
  AdsClientMock();
  ~AdsClientMock() override;

  MOCK_CONST_METHOD0(IsEnabled, bool());

  MOCK_CONST_METHOD0(ShouldAllowAdConversionTracking, bool());

  MOCK_CONST_METHOD0(GetLocale, std::string());

  MOCK_CONST_METHOD0(GetAdsPerHour, uint64_t());

  MOCK_CONST_METHOD0(GetAdsPerDay, uint64_t());

  MOCK_CONST_METHOD0(ShouldAllowAdsSubdivisionTargeting, bool());

  MOCK_METHOD1(SetAllowAdsSubdivisionTargeting, void(
      const bool should_allow));

  MOCK_CONST_METHOD0(GetAdsSubdivisionTargetingCode, std::string());

  MOCK_METHOD1(SetAdsSubdivisionTargetingCode, void(
      const std::string& subdivision_targeting_code));

  MOCK_CONST_METHOD0(GetAutomaticallyDetectedAdsSubdivisionTargetingCode,
      std::string());

  MOCK_METHOD1(SetAutomaticallyDetectedAdsSubdivisionTargetingCode, void(
      const std::string& subdivision_targeting_code));

  MOCK_METHOD1(SetIdleThreshold, void(
      const int threshold));

  MOCK_CONST_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD0(IsForeground, bool());

  MOCK_CONST_METHOD0(CanShowBackgroundNotifications, bool());

  MOCK_METHOD1(ShowNotification, void(
      std::unique_ptr<AdNotificationInfo> info));

  MOCK_METHOD0(ShouldShowNotifications, bool());

  MOCK_METHOD1(CloseNotification, void(
      const std::string& uuid));

  MOCK_METHOD2(UrlRequest, void(
      UrlRequestPtr url_request,
      UrlRequestCallback callback));

  MOCK_METHOD3(Save, void(
      const std::string& name,
      const std::string& value,
      ResultCallback callback));

  MOCK_METHOD2(LoadUserModelForId, void(
      const std::string& id,
      LoadCallback callback));

  MOCK_METHOD2(Load, void(
      const std::string& name,
      LoadCallback callback));

  MOCK_METHOD1(LoadResourceForId, std::string(
      const std::string& id));

  MOCK_METHOD2(RunDBTransaction, void(
      DBTransactionPtr,
      RunDBTransactionCallback));

  MOCK_METHOD0(OnAdRewardsChanged, void());

  MOCK_METHOD4(Log, void(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message));
};

}  // namespace ads

#endif  // BAT_ADS_TEST_ADS_CLIENT_MOCK_H_
