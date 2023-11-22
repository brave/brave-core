/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRequestSignedTokensUrlRequestBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsRequestSignedTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  const std::vector<cbr::Token> tokens = test::BuildTokens(/*count=*/3);
  const std::vector<cbr::BlindedToken> blinded_tokens =
      cbr::BlindTokens(tokens);

  RequestSignedTokensUrlRequestBuilder url_request_builder(test::GetWallet(),
                                                           blinded_tokens);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://mywallet.ads.bravesoftware.com/v3/confirmation/token/"
      "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7");
  expected_url_request->headers = {
      "digest: SHA-256=dbSPIf2biUcc5mfr0b3dlYtVqnyelAFh1LBD6TjnXZc=",
      R"(signature: keyId="primary",algorithm="ed25519",headers="digest",signature="lyFlFeZ4+u1DnQSbf2rijak+ezjJzpcZbA9c0uiUcz1t9rSgVwQvBnRRyju+jj5ysFcdNSWjj5csJ0vCbNlGAQ==")",
      "content-type: application/json", "accept: application/json"};
  expected_url_request->content =
      R"({"blindedTokens":["Ev5JE4/9TZI/5TqyN9JWfJ1To0HBwQw2rWeAPcdjX3Q=","shDzMRNpQKrQAfRctVm4l0Ulaoek0spX8iabH1+Vx00=","kMI3fgomSSNcT1N8d3b+AlZXybqA3st3Ks6XhwaSRF4="]})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;
  EXPECT_EQ(url_request, expected_url_request);
}

}  // namespace brave_ads
