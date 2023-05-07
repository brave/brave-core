/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

#include <ostream>
#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

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
      is_mobile = false;
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
      is_mobile = false;
      name = "linux";
      break;
    }

    case PlatformType::kMacOS: {
      is_mobile = false;
      name = "macos";
      break;
    }

    case PlatformType::kWindows: {
      is_mobile = false;
      name = "windows";
      break;
    }
  }

  ON_CALL(mock, IsMobile()).WillByDefault(Return(is_mobile));
  ON_CALL(mock, GetName()).WillByDefault(Return(name));
  ON_CALL(mock, GetType()).WillByDefault(Return(type));
}

void MockIsNetworkConnectionAvailable(const AdsClientMock& mock,
                                      const bool is_available) {
  ON_CALL(mock, IsNetworkConnectionAvailable())
      .WillByDefault(Return(is_available));
}

void MockIsBrowserActive(const AdsClientMock& mock,
                         const bool is_browser_active) {
  ON_CALL(mock, IsBrowserActive()).WillByDefault(Return(is_browser_active));
}

void MockIsBrowserInFullScreenMode(const AdsClientMock& mock,
                                   const bool is_browser_in_full_screen_mode) {
  ON_CALL(mock, IsBrowserInFullScreenMode())
      .WillByDefault(Return(is_browser_in_full_screen_mode));
}

void MockCanShowNotificationAds(AdsClientMock& mock, const bool can_show) {
  ON_CALL(mock, CanShowNotificationAds()).WillByDefault(Return(can_show));
}

void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& mock,
    const bool can_show) {
  ON_CALL(mock, CanShowNotificationAdsWhileBrowserIsBackgrounded())
      .WillByDefault(Return(can_show));
}

void MockGetBrowsingHistory(AdsClientMock& mock,
                            const std::vector<GURL>& history) {
  ON_CALL(mock, GetBrowsingHistory(_, _, _))
      .WillByDefault(
          Invoke([history](const size_t max_count, const size_t /*days_ago*/,
                           GetBrowsingHistoryCallback callback) {
            CHECK_LE(history.size(), max_count);

            std::move(callback).Run(history);
          }));
}

void MockUrlResponses(AdsClientMock& mock,
                      const URLResponseMap& url_responses) {
  ON_CALL(mock, UrlRequest(_, _))
      .WillByDefault(
          Invoke([url_responses](const mojom::UrlRequestInfoPtr& url_request,
                                 UrlRequestCallback callback) {
            const absl::optional<mojom::UrlResponseInfo> url_response =
                GetNextUrlResponseForRequest(url_request, url_responses);
            if (!url_response) {
              // URL request should not be mocked.
              return std::move(callback).Run(/*url_response*/ {});
            }

            std::move(callback).Run(*url_response);
          }));
}

}  // namespace brave_ads
