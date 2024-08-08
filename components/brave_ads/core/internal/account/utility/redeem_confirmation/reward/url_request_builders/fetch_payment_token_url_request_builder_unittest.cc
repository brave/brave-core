/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsFetchPaymentTokenUrlRequestBuilderTest : public test::TestBase {};

TEST_F(BraveAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  FetchPaymentTokenUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  const mojom::UrlRequestInfoPtr expected_url_request =
      mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://anonymous.ads.bravesoftware.com/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken)");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;
  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace brave_ads
