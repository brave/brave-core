/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"

#include <cstddef>
#include <optional>
#include <utility>

#include "brave/components/brave_ads/core/internal/common/test/internal/url_response_test_util_internal.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads::test {

void MockIsNetworkConnectionAvailable(const AdsClientMock& ads_client_mock,
                                      bool is_available) {
  ON_CALL(ads_client_mock, IsNetworkConnectionAvailable())
      .WillByDefault(::testing::Return(is_available));
}

void MockIsBrowserActive(const AdsClientMock& ads_client_mock, bool is_active) {
  ON_CALL(ads_client_mock, IsBrowserActive)
      .WillByDefault(::testing::Return(is_active));
}

void MockIsBrowserInFullScreenMode(const AdsClientMock& ads_client_mock,
                                   bool is_full_screen_mode) {
  ON_CALL(ads_client_mock, IsBrowserInFullScreenMode())
      .WillByDefault(::testing::Return(is_full_screen_mode));
}

void MockCanShowNotificationAds(const AdsClientMock& ads_client_mock,
                                bool can_show) {
  ON_CALL(ads_client_mock, CanShowNotificationAds())
      .WillByDefault(::testing::Return(can_show));
}

void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& ads_client_mock,
    bool can_show) {
  ON_CALL(ads_client_mock, CanShowNotificationAdsWhileBrowserIsBackgrounded())
      .WillByDefault(::testing::Return(can_show));
}

void MockGetSiteHistory(const AdsClientMock& ads_client_mock,
                        const SiteHistoryList& site_history) {
  ON_CALL(ads_client_mock, GetSiteHistory)
      .WillByDefault([site_history](size_t max_count,
                                    size_t /*recent_day_range*/,
                                    GetSiteHistoryCallback callback) {
        CHECK_LE(site_history.size(), max_count);

        std::move(callback).Run(site_history);
      });
}

void MockUrlResponses(const AdsClientMock& ads_client_mock,
                      const URLResponseMap& url_responses) {
  ON_CALL(ads_client_mock, UrlRequest)
      .WillByDefault(
          [url_responses](const mojom::UrlRequestInfoPtr& mojom_url_request,
                          UrlRequestCallback callback) {
            std::optional<mojom::UrlResponseInfo> url_response =
                GetNextUrlResponseForRequest(mojom_url_request, url_responses);
            if (!url_response) {
              // URL request should not be mocked.
              return std::move(callback).Run(/*url_response=*/{});
            }

            std::move(callback).Run(*url_response);
          });
}

}  // namespace brave_ads::test
