/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/network/oblivious_http_key_config.h"

#include <cstddef>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/browser/network/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr std::string_view kKeyConfigUrl = "https://example.com/key-config";
constexpr std::string_view kKeyConfig = "key-config";
constexpr std::string_view kStaleKeyConfig = "stale-key-config";

}  // namespace

class BraveAdsObliviousHttpKeyConfigTest : public testing::Test {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kAdsObliviousHttpFeature, {{"key_config_expires_after", "3d"},
                                   {"key_config_initial_backoff_delay", "1h"},
                                   {"key_config_max_backoff_delay", "1d"}});

    prefs_.registry()->RegisterStringPref(prefs::kObliviousHttpKeyConfig, "");
    prefs_.registry()->RegisterTimePref(prefs::kObliviousHttpKeyConfigExpiresAt,
                                        base::Time());
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::test::TaskEnvironment task_environment_;

  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_{
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &url_loader_factory_)};
};

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       GetReturnsNulloptWhenNoConfigCached) {
  // Arrange
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Assert
  EXPECT_FALSE(ohttp_key_config.Get());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       MaybeFetchStartsKeyConfigFetchImmediatelyWhenKeyConfigIsMissing) {
  // Arrange
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.MaybeFetch();

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       MaybeFetchDoesNotFetchImmediatelyWhenKeyConfigIsFresh) {
  // Arrange
  prefs_.SetTime(prefs::kObliviousHttpKeyConfigExpiresAt,
                 base::Time::Now() + base::Days(1));
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.MaybeFetch();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest, SuccessfulFetchSetsConfig) {
  // Arrange
  url_loader_factory_.AddResponse(kKeyConfigUrl, kKeyConfig);
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.MaybeFetch();

  // Assert
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ohttp_key_config.Get().has_value(); }));
  EXPECT_THAT(ohttp_key_config.Get(), ::testing::Optional(kKeyConfig));
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       FailedFetchDueToErrorStatusDoesNotCacheConfig) {
  // Arrange
  url_loader_factory_.AddResponse(kKeyConfigUrl, /*content=*/"",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.MaybeFetch();

  // Assert
  EXPECT_FALSE(ohttp_key_config.Get());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       FailedFetchDueToEmptyBodyDoesNotCacheConfig) {
  // Arrange
  url_loader_factory_.AddResponse(kKeyConfigUrl, /*content=*/"");
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.MaybeFetch();

  // Assert
  EXPECT_FALSE(ohttp_key_config.Get());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest, RefetchClearsCachedConfig) {
  // Arrange
  url_loader_factory_.AddResponse(kKeyConfigUrl, kKeyConfig);
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));
  ohttp_key_config.MaybeFetch();
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ohttp_key_config.Get().has_value(); }));
  ASSERT_THAT(ohttp_key_config.Get(), ::testing::Optional(kKeyConfig));

  // Act
  ohttp_key_config.Refetch();

  // Assert
  EXPECT_FALSE(ohttp_key_config.Get());
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest, RefetchStartsNewFetch) {
  // Arrange
  url_loader_factory_.AddResponse(kKeyConfigUrl, kKeyConfig);
  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));

  // Act
  ohttp_key_config.Refetch();

  // Assert
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ohttp_key_config.Get().has_value(); }));
  EXPECT_THAT(ohttp_key_config.Get(), ::testing::Optional(kKeyConfig));
}

TEST_F(BraveAdsObliviousHttpKeyConfigTest,
       RefetchCancelsInFlightFetchSoStaleResponseIsDiscarded) {
  // Arrange
  size_t request_count = 0U;
  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&](const network::ResourceRequest& resource_request) {
        ++request_count;
        url_loader_factory_.AddResponse(
            resource_request.url.spec(),
            request_count == 1U ? kStaleKeyConfig : kKeyConfig);
      }));

  ObliviousHttpKeyConfig ohttp_key_config(prefs_, shared_url_loader_factory_,
                                          GURL(kKeyConfigUrl));
  ohttp_key_config.MaybeFetch();
  ASSERT_EQ(1U, request_count);

  // Act
  ohttp_key_config.Refetch();

  // Assert
  EXPECT_EQ(2U, request_count);
  ASSERT_TRUE(
      base::test::RunUntil([&] { return ohttp_key_config.Get().has_value(); }));
  EXPECT_THAT(ohttp_key_config.Get(), ::testing::Optional(kKeyConfig));
}

}  // namespace brave_ads
