/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_search/renderer/backup_results_url_loader_throttle.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search/common/features.h"
#include "services/network/public/cpp/resource_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "url/gurl.h"

namespace brave_search {

class BackupResultsURLLoaderThrottleTest
    : public testing::Test,
      public blink::URLLoaderThrottle::Delegate {
 public:
  void SetUp() override {
    throttle_ = std::make_unique<BackupResultsURLLoaderThrottle>();
    throttle_->set_delegate(this);
  }

  // blink::URLLoaderThrottle::Delegate
  void CancelWithError(int error_code,
                       std::string_view custom_reason) override {
    canceled_ = true;
  }
  void Resume() override {}

 protected:
  bool RunThrottle(std::string_view url) {
    canceled_ = false;
    network::ResourceRequest request;
    request.url = GURL(url);
    bool defer = false;
    throttle_->WillStartRequest(&request, &defer);
    return canceled_;
  }

  std::unique_ptr<BackupResultsURLLoaderThrottle> throttle_;
  bool canceled_ = false;
};

// Requests to / and /search* are always allowed regardless of params.
TEST_F(BackupResultsURLLoaderThrottleTest, AlwaysAllowedPaths) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowFetches.name, "false"},
       {features::kBackupResultsAllowCosmeticAssets.name, "false"},
       {features::kBackupResultsAllowUnclassifiedRequests.name, "false"}});

  EXPECT_FALSE(RunThrottle("https://www.google.com/"));
  EXPECT_FALSE(RunThrottle("https://www.google.com/search?q=test"));
  EXPECT_FALSE(RunThrottle("https://www.google.com/search"));
}

// Fetch paths are cancelled when allow_fetches is false.
TEST_F(BackupResultsURLLoaderThrottleTest, FetchPathsBlockedByDefault) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowFetches.name, "false"}});

  EXPECT_TRUE(RunThrottle("https://www.google.com/gen_204"));
  EXPECT_TRUE(RunThrottle("https://www.google.com/client_204"));
  EXPECT_TRUE(RunThrottle("https://www.google.com/complete/s?q=test"));
}

// Fetch paths are allowed when allow_fetches is true.
TEST_F(BackupResultsURLLoaderThrottleTest, FetchPathsAllowedWhenParamTrue) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowFetches.name, "true"}});

  EXPECT_FALSE(RunThrottle("https://www.google.com/gen_204"));
  EXPECT_FALSE(RunThrottle("https://www.google.com/client_204"));
  EXPECT_FALSE(RunThrottle("https://www.google.com/complete/s?q=test"));
}

// Cosmetic asset paths are cancelled when allow_cosmetic_assets is false.
TEST_F(BackupResultsURLLoaderThrottleTest, CosmeticAssetsBlockedByDefault) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowCosmeticAssets.name, "false"}});

  constexpr std::string_view kBlockedUrls[] = {
      "https://www.google.com/logo.png",
      "https://www.google.com/font.woff2",
      "https://www.google.com/favicon.ico",
      "https://www.google.com/images/branding/logo.png",
      "https://www.google.com/shopping/ads",
      "https://www.google.com/favicon",
      "https://www.google.com/grass",
  };
  for (const auto url : kBlockedUrls) {
    EXPECT_TRUE(RunThrottle(url)) << "Expected cancel for: " << url;
  }
}

// Cosmetic assets are allowed when allow_cosmetic_assets is true.
TEST_F(BackupResultsURLLoaderThrottleTest, CosmeticAssetsAllowedWhenParamTrue) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowCosmeticAssets.name, "true"}});

  constexpr std::string_view kAllowedUrls[] = {
      "https://www.google.com/logo.png",
      "https://www.google.com/font.woff2",
      "https://www.google.com/favicon.ico",
      "https://www.google.com/images/branding/logo.png",
      "https://www.google.com/shopping/ads",
  };
  for (const auto url : kAllowedUrls) {
    EXPECT_FALSE(RunThrottle(url)) << "Expected allow for: " << url;
  }
}

// Unclassified requests are cancelled when allow_unclassified_requests is
// false.
TEST_F(BackupResultsURLLoaderThrottleTest,
       UnclassifiedRequestsBlockedByDefault) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowUnclassifiedRequests.name, "false"}});

  EXPECT_TRUE(RunThrottle("https://www.google.com/some/random/path"));
  EXPECT_TRUE(RunThrottle("https://www.google.com/xjs/_/js/main.js"));
}

// Unclassified requests are allowed when allow_unclassified_requests is true.
TEST_F(BackupResultsURLLoaderThrottleTest,
       UnclassifiedRequestsAllowedWhenParamTrue) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowUnclassifiedRequests.name, "true"}});

  EXPECT_FALSE(RunThrottle("https://www.google.com/some/random/path"));
}

// Cosmetic asset requests are governed by allow_cosmetic_assets, not
// allow_unclassified_requests.
TEST_F(BackupResultsURLLoaderThrottleTest, CosmeticIsClassified) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowCosmeticAssets.name, "true"},
       {features::kBackupResultsAllowUnclassifiedRequests.name, "false"}});

  EXPECT_FALSE(RunThrottle("https://www.google.com/logo.png"));
}

// Fetch requests are governed by allow_fetches, not
// allow_unclassified_requests.
TEST_F(BackupResultsURLLoaderThrottleTest, FetchIsClassified) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeatureWithParameters(
      features::kBackupResults,
      {{features::kBackupResultsAllowFetches.name, "true"},
       {features::kBackupResultsAllowUnclassifiedRequests.name, "false"}});

  EXPECT_FALSE(RunThrottle("https://www.google.com/gen_204"));
}

}  // namespace brave_search
