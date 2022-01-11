/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"

#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsFetchPaymentTokenUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsFetchPaymentTokenUrlRequestBuilderTest() = default;

  ~BatAdsFetchPaymentTokenUrlRequestBuilderTest() override = default;
};

TEST_F(BatAdsFetchPaymentTokenUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  privacy::SetUnblindedTokens(1);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.bravesoftware.com/v2/confirmation/d990ed8d-d739-49fb-811b-c2e02158fb60/paymentToken)";
  expected_url_request->method = mojom::UrlRequestMethod::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
