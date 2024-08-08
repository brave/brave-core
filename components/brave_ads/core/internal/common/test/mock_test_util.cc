/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/url_response_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads::test {

namespace {

constexpr char kNightlyBuildChannelName[] = "nightly";
constexpr char kBetaBuildChannelName[] = "beta";
constexpr char kReleaseBuildChannelName[] = "release";

constexpr char kUnknownPlatformType[] = "unknown";
constexpr char kAndroidPlatformType[] = "android";
constexpr char kIOSPlatformType[] = "ios";
constexpr char kLinuxPlatformType[] = "linux";
constexpr char kMacOSPlatformType[] = "macos";
constexpr char kWindowsPlatformType[] = "windows";

}  // namespace

void MockDeviceId() {
  CHECK(GlobalState::HasInstance());

  GlobalState::GetInstance()->SysInfo().device_id = kDeviceId;
}

void MockPlatformHelper(const PlatformHelperMock& platform_helper_mock,
                        const PlatformType type) {
  PlatformHelper::SetForTesting(&platform_helper_mock);

  bool is_mobile = false;
  std::string name;

  switch (type) {
    case PlatformType::kUnknown: {
      name = kUnknownPlatformType;
      break;
    }

    case PlatformType::kAndroid: {
      is_mobile = true;
      name = kAndroidPlatformType;
      break;
    }

    case PlatformType::kIOS: {
      is_mobile = true;
      name = kIOSPlatformType;
      break;
    }

    case PlatformType::kLinux: {
      name = kLinuxPlatformType;
      break;
    }

    case PlatformType::kMacOS: {
      name = kMacOSPlatformType;
      break;
    }

    case PlatformType::kWindows: {
      name = kWindowsPlatformType;
      break;
    }
  }

  ON_CALL(platform_helper_mock, IsMobile)
      .WillByDefault(::testing::Return(is_mobile));
  ON_CALL(platform_helper_mock, GetName).WillByDefault(::testing::Return(name));
  ON_CALL(platform_helper_mock, GetType).WillByDefault(::testing::Return(type));
}

void MockBuildChannel(const BuildChannelType type) {
  CHECK(GlobalState::HasInstance());

  auto& build_channel = GlobalState::GetInstance()->BuildChannel();
  switch (type) {
    case BuildChannelType::kNightly: {
      build_channel.is_release = false;
      build_channel.name = kNightlyBuildChannelName;
      return;
    }

    case BuildChannelType::kBeta: {
      build_channel.is_release = false;
      build_channel.name = kBetaBuildChannelName;
      return;
    }

    case BuildChannelType::kRelease: {
      build_channel.is_release = true;
      build_channel.name = kReleaseBuildChannelName;
      return;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for BuildChannelType: "
                        << base::to_underlying(type);
}

void MockIsNetworkConnectionAvailable(const AdsClientMock& ads_client_mock,
                                      const bool is_available) {
  ON_CALL(ads_client_mock, IsNetworkConnectionAvailable())
      .WillByDefault(::testing::Return(is_available));
}

void MockIsBrowserActive(const AdsClientMock& ads_client_mock,
                         const bool is_active) {
  ON_CALL(ads_client_mock, IsBrowserActive)
      .WillByDefault(::testing::Return(is_active));
}

void MockIsBrowserInFullScreenMode(const AdsClientMock& ads_client_mock,
                                   const bool is_full_screen_mode) {
  ON_CALL(ads_client_mock, IsBrowserInFullScreenMode())
      .WillByDefault(::testing::Return(is_full_screen_mode));
}

void MockCanShowNotificationAds(const AdsClientMock& ads_client_mock,
                                const bool can_show) {
  ON_CALL(ads_client_mock, CanShowNotificationAds())
      .WillByDefault(::testing::Return(can_show));
}

void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& ads_client_mock,
    const bool can_show) {
  ON_CALL(ads_client_mock, CanShowNotificationAdsWhileBrowserIsBackgrounded())
      .WillByDefault(::testing::Return(can_show));
}

void MockGetSiteHistory(const AdsClientMock& ads_client_mock,
                        const SiteHistoryList& site_history) {
  ON_CALL(ads_client_mock, GetSiteHistory)
      .WillByDefault(
          ::testing::Invoke([site_history](const size_t max_count,
                                           const size_t /*recent_day_range*/,
                                           GetSiteHistoryCallback callback) {
            CHECK_LE(site_history.size(), max_count);

            std::move(callback).Run(site_history);
          }));
}

void MockUrlResponses(const AdsClientMock& ads_client_mock,
                      const URLResponseMap& url_responses) {
  ON_CALL(ads_client_mock, UrlRequest)
      .WillByDefault(::testing::Invoke(
          [url_responses](const mojom::UrlRequestInfoPtr& url_request,
                          UrlRequestCallback callback) {
            const std::optional<mojom::UrlResponseInfo> url_response =
                GetNextUrlResponseForRequest(url_request, url_responses);
            if (!url_response) {
              // URL request should not be mocked.
              return std::move(callback).Run(/*url_response=*/{});
            }

            std::move(callback).Run(*url_response);
          }));
}

}  // namespace brave_ads::test
