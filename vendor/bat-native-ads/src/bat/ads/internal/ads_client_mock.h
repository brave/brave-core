/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_
#define BAT_ADS_INTERNAL_ADS_CLIENT_MOCK_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <ostream>

#include "bat/ads/ads_client.h"
#include "bat/ads/ads.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class MockLogStreamImpl : public LogStream {
 public:
  MockLogStreamImpl(
      const char* file,
      const int line,
      const LogLevel log_level);

  std::ostream& stream() override;

 private:
  // Not copyable, not assignable
  MockLogStreamImpl(const MockLogStreamImpl&) = delete;
  MockLogStreamImpl& operator=(const MockLogStreamImpl&) = delete;
};

class MockAdsClient : public AdsClient {
 public:
  MockAdsClient();
  ~MockAdsClient() override;

  MOCK_CONST_METHOD0(IsEnabled, bool());

  MOCK_CONST_METHOD0(ShouldAllowAdConversionTracking, bool());

  MOCK_CONST_METHOD0(GetLocale, std::string());

  MOCK_CONST_METHOD0(GetAdsPerHour, uint64_t());

  MOCK_CONST_METHOD0(GetAdsPerDay, uint64_t());

  MOCK_METHOD1(SetIdleThreshold, void(
      const int threshold));

  MOCK_CONST_METHOD0(IsNetworkConnectionAvailable, bool());

  MOCK_CONST_METHOD1(GetClientInfo, void(
      ClientInfo* info));

  MOCK_CONST_METHOD0(GetUserModelLanguages, std::vector<std::string>());

  MOCK_CONST_METHOD2(LoadUserModelForLanguage, void(
      const std::string& language,
      LoadCallback callback));

  MOCK_CONST_METHOD0(IsForeground, bool());

  MOCK_METHOD0(ShouldShowNotifications, bool());

  MOCK_METHOD1(ShowNotification, void(
      std::unique_ptr<AdNotificationInfo> info));

  MOCK_METHOD1(CloseNotification, void(
      const std::string& uuid));

  MOCK_METHOD1(SetCatalogIssuers, void(
      std::unique_ptr<IssuersInfo> info));

  MOCK_METHOD2(ConfirmAd, void(
      const AdInfo& info,
      const ConfirmationType confirmation_type));

  MOCK_METHOD3(ConfirmAction, void(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ConfirmationType confirmation_type));

  MOCK_METHOD1(SetTimer, uint32_t(
      const uint64_t time_offset));

  MOCK_METHOD1(KillTimer, void(
      uint32_t timer_id));

  MOCK_METHOD6(URLRequest, void(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback));

  MOCK_METHOD3(Save, void(
      const std::string& name,
      const std::string& value,
      ResultCallback callback));

  MOCK_METHOD2(SaveBundleState, void(
      std::unique_ptr<BundleState> state,
      ResultCallback callback));

  MOCK_METHOD2(Load, void(
      const std::string& name,
      LoadCallback callback));

  MOCK_METHOD1(LoadJsonSchema, std::string(
      const std::string& name));

  MOCK_METHOD1(LoadSampleBundle, void(
      LoadSampleBundleCallback callback));

  MOCK_METHOD2(Reset, void(
      const std::string& name,
      ResultCallback callback));

  MOCK_METHOD2(GetCreativeAdNotifications, void(
      const std::vector<std::string>& categories,
      GetCreativeAdNotificationsCallback callback));

  MOCK_METHOD1(GetAdConversions, void(
      GetAdConversionsCallback callback));

  MOCK_CONST_METHOD1(EventLog, void(
      const std::string& json));

  MOCK_CONST_METHOD0(CanShowBackgroundNotifications, bool());

  std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const;
};

}  // namespace ads

#endif  // BAT_ADS_TEST_ADS_CLIENT_MOCK_H_
