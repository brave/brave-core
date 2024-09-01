/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kExpectedUrl[] =
    R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a)";
constexpr char kExpectedUrlRequestContent[] =
    R"({"creativeInstanceId":"546fe7b0-5047-4f28-a11c-81f14edcf0f6","transactionId":"8b742869-6e4a-490c-ac31-31b49130098a","type":"view"})";

}  // namespace

class BraveAdsCreateNonRewardConfirmationUrlRequestBuilderTest
    : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::DisableBraveRewards();
  }
};

TEST_F(BraveAdsCreateNonRewardConfirmationUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  CreateNonRewardConfirmationUrlRequestBuilder url_request_builder(
      *confirmation);

  // Act
  const mojom::UrlRequestInfoPtr mojom_url_request =
      url_request_builder.Build();

  // Assert
  const mojom::UrlRequestInfoPtr expected_mojom_url_request =
      mojom::UrlRequestInfo::New();
  expected_mojom_url_request->url = GURL(kExpectedUrl);
  expected_mojom_url_request->headers = {"accept: application/json"};
  expected_mojom_url_request->content = kExpectedUrlRequestContent;
  expected_mojom_url_request->content_type = "application/json";
  expected_mojom_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(expected_mojom_url_request, mojom_url_request);
}

}  // namespace brave_ads
