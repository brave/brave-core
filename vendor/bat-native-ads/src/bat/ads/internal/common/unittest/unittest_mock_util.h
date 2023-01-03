/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/common/platform/platform_helper_mock.h"
#include "bat/ads/internal/common/unittest/unittest_build_channel_types.h"
#include "bat/ads/internal/common/unittest/unittest_url_response_alias.h"

namespace ads {

class Database;

void MockBuildChannel(BuildChannelType type);

void MockPlatformHelper(const std::unique_ptr<PlatformHelperMock>& mock,
                        PlatformType type);

void MockIsNetworkConnectionAvailable(
    const std::unique_ptr<AdsClientMock>& mock,
    bool is_available);

void MockIsBrowserActive(const std::unique_ptr<AdsClientMock>& mock,
                         bool is_browser_active);
void MockIsBrowserInFullScreenMode(const std::unique_ptr<AdsClientMock>& mock,
                                   bool is_browser_in_full_screen_mode);

void MockCanShowNotificationAds(const std::unique_ptr<AdsClientMock>& mock,
                                bool can_show);
void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const std::unique_ptr<AdsClientMock>& mock,
    bool can_show);
void MockShowNotificationAd(const std::unique_ptr<AdsClientMock>& mock);
void MockCloseNotificationAd(const std::unique_ptr<AdsClientMock>& mock);

void MockRecordAdEventForId(const std::unique_ptr<AdsClientMock>& mock);
void MockGetAdEventHistory(const std::unique_ptr<AdsClientMock>& mock);
void MockResetAdEventHistoryForId(const std::unique_ptr<AdsClientMock>& mock);

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock);

void MockUrlResponses(const std::unique_ptr<AdsClientMock>& mock,
                      const URLResponseMap& url_responses);

void MockSave(const std::unique_ptr<AdsClientMock>& mock);
void MockLoad(const std::unique_ptr<AdsClientMock>& mock,
              const base::ScopedTempDir& temp_dir);
void MockLoadFileResource(const std::unique_ptr<AdsClientMock>& mock,
                          const base::ScopedTempDir& temp_dir);
void MockLoadDataResource(const std::unique_ptr<AdsClientMock>& mock);

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database);

void MockGetBooleanPref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetBooleanPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetIntegerPref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetIntegerPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetDoublePref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetDoublePref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetStringPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetInt64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetInt64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetUint64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetUint64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetTimePref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetTimePref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetDictPref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetDictPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetListPref(const std::unique_ptr<AdsClientMock>& mock);
void MockSetListPref(const std::unique_ptr<AdsClientMock>& mock);
void MockClearPref(const std::unique_ptr<AdsClientMock>& mock);
void MockHasPrefPath(const std::unique_ptr<AdsClientMock>& mock);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
