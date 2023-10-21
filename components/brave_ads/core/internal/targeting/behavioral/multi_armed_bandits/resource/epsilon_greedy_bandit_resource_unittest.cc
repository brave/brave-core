/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource.h"

#include <memory>

#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEpsilonGreedyBanditResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    catalog_ = std::make_unique<Catalog>();
    resource_ = std::make_unique<EpsilonGreedyBanditResource>(*catalog_);
  }

  void LoadResource(const std::string& catalog) {
    const URLResponseMap url_responses = {
        {BuildCatalogUrlPath(),
         {{net::HTTP_OK,
           /*response_body=*/base::StrCat({"/", catalog})}}}};
    MockUrlResponses(ads_client_mock_, url_responses);

    NotifyDidInitializeAds();
  }

  std::unique_ptr<Catalog> catalog_;

  std::unique_ptr<EpsilonGreedyBanditResource> resource_;
};

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest, IsNotInitialized) {
  // Act & Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       LoadResourceIfNotificationAdsAndBraveNewsAdsAreEnabled) {
  // Act
  LoadResource("catalog.json");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       LoadResourceIfOptedOutOfNotificationAdsAndOptedInToBraveNewsAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  // Act
  LoadResource("catalog.json");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       LoadResourceIfOptedInToNotificationAdsAndOptedOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  // Act
  LoadResource("catalog.json");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest, LoadResourceIfEmptyCatalog) {
  // Act
  LoadResource("empty_catalog.json");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsEpsilonGreedyBanditResourceTest,
       DoNotLoadResourceIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  test::OptOutOfNotificationAds();
  test::OptOutOfBraveNewsAds();

  // Act
  LoadResource("catalog.json");

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsEpsilonGreedyBanditResourceTest,
    ResetResourceWhenOptedInToNotificationAdsPrefDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  LoadResource("catalog.json");

  test::OptOutOfNotificationAds();
  test::OptOutOfBraveNewsAds();

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsEpsilonGreedyBanditResourceTest,
    DoNotResetResourceWhenOptedInToNotificationAdsPrefDidChangeIfNotificationAdsOrBraveNewsAdsAreEnabled) {
  // Arrange
  LoadResource("catalog.json");

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

}  // namespace brave_ads
