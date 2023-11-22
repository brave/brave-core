/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

void MockDeviceId() {
  CHECK(GlobalState::HasInstance());

  GlobalState::GetInstance()->SysInfo().device_id = kDeviceId;
}

void MockBuildChannel(const BuildChannelType type) {
  CHECK(GlobalState::HasInstance());

  auto& build_channel = GlobalState::GetInstance()->BuildChannel();
  switch (type) {
    case BuildChannelType::kNightly: {
      build_channel.is_release = false;
      build_channel.name = "nightly";
      return;
    }

    case BuildChannelType::kBeta: {
      build_channel.is_release = false;
      build_channel.name = "beta";
      return;
    }

    case BuildChannelType::kRelease: {
      build_channel.is_release = true;
      build_channel.name = "release";
      return;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for BuildChannelType: "
                        << static_cast<int>(type);
}

void MockPlatformHelper(const PlatformHelperMock& mock,
                        const PlatformType type) {
  PlatformHelper::SetForTesting(&mock);

  bool is_mobile = false;
  std::string name;

  switch (type) {
    case PlatformType::kUnknown: {
      name = "unknown";
      break;
    }

    case PlatformType::kAndroid: {
      is_mobile = true;
      name = "android";
      break;
    }

    case PlatformType::kIOS: {
      is_mobile = true;
      name = "ios";
      break;
    }

    case PlatformType::kLinux: {
      name = "linux";
      break;
    }

    case PlatformType::kMacOS: {
      name = "macos";
      break;
    }

    case PlatformType::kWindows: {
      name = "windows";
      break;
    }
  }

  ON_CALL(mock, IsMobile()).WillByDefault(::testing::Return(is_mobile));
  ON_CALL(mock, GetName()).WillByDefault(::testing::Return(name));
  ON_CALL(mock, GetType()).WillByDefault(::testing::Return(type));
}

void MockIsNetworkConnectionAvailable(const AdsClientMock& mock,
                                      const bool is_available) {
  ON_CALL(mock, IsNetworkConnectionAvailable())
      .WillByDefault(::testing::Return(is_available));
}

void MockIsBrowserActive(const AdsClientMock& mock, const bool is_active) {
  ON_CALL(mock, IsBrowserActive()).WillByDefault(::testing::Return(is_active));
}

void MockIsBrowserInFullScreenMode(const AdsClientMock& mock,
                                   const bool is_full_screen_mode) {
  ON_CALL(mock, IsBrowserInFullScreenMode())
      .WillByDefault(::testing::Return(is_full_screen_mode));
}

void MockCanShowNotificationAds(AdsClientMock& mock, const bool can_show) {
  ON_CALL(mock, CanShowNotificationAds())
      .WillByDefault(::testing::Return(can_show));
}

void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& mock,
    const bool can_show) {
  ON_CALL(mock, CanShowNotificationAdsWhileBrowserIsBackgrounded())
      .WillByDefault(::testing::Return(can_show));
}

void MockGetBrowsingHistory(AdsClientMock& mock,
                            const std::vector<GURL>& history) {
  ON_CALL(mock, GetBrowsingHistory)
      .WillByDefault(::testing::Invoke(
          [history](const size_t max_count, const size_t /*recent_day_range=*/,
                    GetBrowsingHistoryCallback callback) {
            CHECK_LE(history.size(), max_count);

            std::move(callback).Run(history);
          }));
}

void MockUrlResponses(AdsClientMock& mock,
                      const URLResponseMap& url_responses) {
  ON_CALL(mock, UrlRequest)
      .WillByDefault(::testing::Invoke(
          [url_responses](const mojom::UrlRequestInfoPtr& url_request,
                          UrlRequestCallback callback) {
            const absl::optional<mojom::UrlResponseInfo> url_response =
                GetNextUrlResponseForRequest(url_request, url_responses);
            if (!url_response) {
              // URL request should not be mocked.
              return std::move(callback).Run(/*url_response=*/{});
            }

            std::move(callback).Run(*url_response);
          }));
}

}  // namespace brave_ads
