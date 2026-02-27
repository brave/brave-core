// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/favicon/core/large_icon_service_impl.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "components/favicon/core/test/mock_favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/image_fetcher/core/mock_image_fetcher.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_scale_factor.h"
#include "url/gurl.h"

namespace favicon {
namespace {

using image_fetcher::MockImageFetcher;
using testing::_;
using testing::HasSubstr;
using testing::NiceMock;
using testing::Not;
using testing::SaveArg;

// Helper that creates a LargeIconServiceImpl, sets up mock expectations to
// capture the server URL, fires a favicon request, and returns the captured
// server URL.
GURL GetServerUrlForPageUrl(const GURL& page_url, bool should_trim_path) {
  base::test::TaskEnvironment task_environment;
  ui::test::ScopedSetSupportedResourceScaleFactors scale_factors(
      {ui::k200Percent});
  NiceMock<MockFaviconService> mock_favicon_service;

  auto mock_image_fetcher = std::make_unique<NiceMock<MockImageFetcher>>();
  GURL actual_server_url;
  EXPECT_CALL(*mock_image_fetcher, FetchImageAndData_(_, _, _, _))
      .WillOnce(SaveArg<0>(&actual_server_url));
  EXPECT_CALL(mock_favicon_service, CanSetOnDemandFavicons(_, _, _))
      .WillOnce([](auto, auto, base::OnceCallback<void(bool)> callback) {
        return base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, base::BindOnce(std::move(callback), true));
      });

  LargeIconServiceImpl large_icon_service(
      &mock_favicon_service, std::move(mock_image_fetcher),
      /*desired_size_in_dip_for_server_requests=*/24,
      /*icon_type_for_server_requests=*/favicon_base::IconType::kTouchIcon,
      /*google_server_client_param=*/"test_chrome");

  large_icon_service
      .GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
          page_url, should_trim_path, TRAFFIC_ANNOTATION_FOR_TESTS,
          favicon_base::GoogleFaviconServerCallback());

  EXPECT_TRUE(
      base::test::RunUntil([&]() { return !actual_server_url.is_empty(); }));
  return actual_server_url;
}

// Google's Favicon Server returns 404 for www.google.com but 200 for
// google.com. Verify that www.google.com is rewritten to google.com.
TEST(BraveLargeIconServiceTest, ShouldStripWwwFromGoogleCom) {
  GURL server_url =
      GetServerUrlForPageUrl(GURL("https://www.google.com/search?q=test"),
                             /*should_trim_path=*/true);
  EXPECT_THAT(server_url.spec(), HasSubstr("url=https://google.com/"));
  EXPECT_THAT(server_url.spec(), Not(HasSubstr("url=https://www.google.com/")));
}

// Verify that www. is NOT stripped from other domains.
TEST(BraveLargeIconServiceTest, ShouldNotStripWwwFromOtherDomains) {
  GURL server_url = GetServerUrlForPageUrl(GURL("https://www.example.com"),
                                           /*should_trim_path=*/false);
  EXPECT_THAT(server_url.spec(), HasSubstr("url=https://www.example.com/"));
}

}  // namespace
}  // namespace favicon
