/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/url_request_builders/fetch_payment_token_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsFetchPaymentTokenUrlRequestBuilderTest : public UnitTestBase {
 protected:
  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_);
  ASSERT_TRUE(confirmation);
  FetchPaymentTokenUrlRequestBuilder url_request_builder(*confirmation);

  // Act
  const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://anonymous.ads.bravesoftware.com/v3/confirmation/"
      "8b742869-6e4a-490c-ac31-31b49130098a/paymentToken");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace brave_ads
