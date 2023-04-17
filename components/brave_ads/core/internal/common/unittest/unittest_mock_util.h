/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mock.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_build_channel_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"

namespace brave_ads {

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

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock);

void MockUrlResponses(const std::unique_ptr<AdsClientMock>& mock,
                      const URLResponseMap& url_responses);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
