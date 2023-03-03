/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/request_signed_tokens_url_request_builder.h"

#include "base/check.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/sys_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::vector<privacy::cbr::Token> GetTokens(const int count) {
  const std::vector<std::string> tokens_base64 = {
      R"(B2CbFJJ1gKJy9qs8NMburYj12VAqnVfFrQ2K2u0QwcBi1YoMMHQfRQeDbOQ62Z+WrCOTYLbZrBY7+j9hz2jLFL74KSQig7/PDbqIpmNYs6PpNUK3MpVc4dm5R9lkySQF)",
      R"(MHbZ2XgFtno4g7yq/tmFCr1sFuFrkE7D6JjVAmM70ZJrwH/EqYNaWL1qANSKXX9ghyiN8KUDThEhDTqhuBQ4v7gzNY2qHav9uiAmjqvLzDp7oxmUBFohmdkVlvWhxV0F)",
      R"(6WWlDOIHNs6Az23V+VM3QTDFFDkR9D0CGZSd27/cjo3eO5EDEzi9Ev5omoJwZQHqiObVgUXmRFRa8UYXsL4O4MvBsYlgGz9VyoBLXo0ethmEBowsrMubj3GR4CQaN6gB)",
      R"(IzhzMBc/rI8uzGuARaudvUYY662c0tqzYDPOfvbWRiThTTyH9fU13nmAmhkdtpoUnDlGTE37fLDpjWPlGdAd9r2qh++09+sa9xHV+V9SXHbr9gtJBybZMWr8vjQuslMM)",
      R"(eZDj3OGto3E0Uz0djk6Ilfgz+Ar4kMAXOL68iLTNycBPgoNnM1rtjaL4OqvSc1ascZhGCf6Js42B/wPVzUYuKMloATKmYs7Ym+ndXnuX0FV9XJs94tlIGcp4k0uOMcgB)",
      R"(8QNMIJuJfu9W4KURg1Y2coXyKjbJOQLmo6RIGg+tKkUcY7srgUpac8XteSwWy6o6YLDoNKXS21FmbZ4VHb+Bv2NVhBWooK0b8lwQAdVUax5+Ej77qK//GeyRmAcAQV8G)",
      R"(6ILvEIM3+kgacI6JFa5415qAdzcg6hccQzEyhMsqYFa3MZKzvcLEF57pFFRoaYw7nFDQL8v8CDG2iSUoBIk8bmeoUwgdXsgofHvSahcBSWawmcnn8ESJTkZPGgxaFgcA)",
      R"(VHDbhwcInhhjL/HhSF+NyYak7Zy24xzDDTpI+3rsEZ7iL4SYUdcVkFmJ+bg8QlmPv8UMTchPBP7CVtCc96jj5PwGMsvAB8t2TffdSK9SHBRx/ZINmYSb7x+GTTdqWugB)",
      R"(YbH2x8oMkQrPR0uX6h8LrcgXSrPlSg60FFfp8V+GM8eiCQTwPJ643kilmlKU/qNZM3e28Hw3W4GPAELnm/YxFzG6qJ4B1wVTBdl/myIa0M3QIdoOn2//+JH2u4jRtIgN)",
      R"(0/KAtyvRoYLhsQnwu4McuG7pglpDpi2BXQi//FwGu8m/O+iTh1Lijzpt2RCnotGh0Wid9efnojrYQH5NJv9GYOhUDX7yYHVjUorc6y6SkUaO1aATc42RciRQ0cmuQFQC)"};

  // Blinded tokens used to create the above tokens:
  // "blindedTokens" : [
  //   "iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=",
  //   "eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=",
  //   "1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8=",
  //   "9gtgRG1Fr6eQAfvIO7qGes2d0Zwnd7EXdOQI9ik0PRE=",
  //   "iGH6L3EtdYLQiD63D/elY3nfI2R8BJzq/ufPtFkTAXg=",
  //   "5mtjGDYwCC54EyFrr/5XoG98Cag7ughIYYr6mp8jmEQ=",
  //   "8vU5KFc8AXn45rcqTGdM9MeUvG+z8RL9o27Lir4izBY=",
  //   "huXHzk2SgmJkMauedoRUr/p86+jh1vKIa93O9FP2PQk=",
  //   "cg9nMhSA7hVoBFbq5rEGVF7kgAoXqMmPApmxO99aGVU=",
  //   "sBJB0ez2qw929moV4PZgw+AVbj7mBj9Mtqy3r2D0kw4="
  // ]

  const int modulo = tokens_base64.size();

  std::vector<privacy::cbr::Token> tokens;
  for (int i = 0; i < count; i++) {
    const std::string& token_base64 = tokens_base64.at(i % modulo);
    const privacy::cbr::Token token = privacy::cbr::Token(token_base64);
    CHECK(token.has_value());

    tokens.push_back(token);
  }

  return tokens;
}

}  // namespace

class BatAdsRequestSignedTokensUrlRequestBuilderTest : public UnitTestBase {};

TEST_F(BatAdsRequestSignedTokensUrlRequestBuilderTest, BuildUrlForRPill) {
  // Arrange
  SysInfo().is_uncertain_future = true;

  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  const std::vector<privacy::cbr::Token> tokens = GetTokens(3);
  const std::vector<privacy::cbr::BlindedToken> blinded_tokens =
      privacy::cbr::BlindTokens(tokens);

  RequestSignedTokensUrlRequestBuilder url_request_builder(
      GetWalletForTesting(), blinded_tokens);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://mywallet.ads.bravesoftware.com/v3/confirmation/token/"
      "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7");
  expected_url_request->headers = {
      "digest: SHA-256=Sxq6H/YDThn/m2RSXsTzewSzKfAuGLh09w7m59VBYwU=",
      R"(signature: keyId="primary",algorithm="ed25519",headers="digest",signature="tLMjZ1f52kBqbwJy0B0On2h82978eV8tf4oK/3UJyq4mQqCu5y2q6puaxoe969ENtwSPU292PvbTIFAZZzwaCA==")",
      "content-type: application/json",
      "Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1)",
      "accept: application/json"};
  expected_url_request->content =
      R"({"blindedTokens":["iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=","eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=","1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8="]})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

TEST_F(BatAdsRequestSignedTokensUrlRequestBuilderTest, BuildUrlForBPill) {
  // Arrange
  SysInfo().is_uncertain_future = false;

  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  const std::vector<privacy::cbr::Token> tokens = GetTokens(3);
  const std::vector<privacy::cbr::BlindedToken> blinded_tokens =
      privacy::cbr::BlindTokens(tokens);

  RequestSignedTokensUrlRequestBuilder url_request_builder(
      GetWalletForTesting(), blinded_tokens);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://mywallet.ads.bravesoftware.com/v3/confirmation/token/"
      "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7");
  expected_url_request->headers = {
      "digest: SHA-256=Sxq6H/YDThn/m2RSXsTzewSzKfAuGLh09w7m59VBYwU=",
      R"(signature: keyId="primary",algorithm="ed25519",headers="digest",signature="tLMjZ1f52kBqbwJy0B0On2h82978eV8tf4oK/3UJyq4mQqCu5y2q6puaxoe969ENtwSPU292PvbTIFAZZzwaCA==")",
      "content-type: application/json",
      "Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1)",
      "accept: application/json"};
  expected_url_request->content =
      R"({"blindedTokens":["iEK4BXJINfAa0kzgpnnukGUAHvH5303+Y/msR5+u/nY=","eAAv7FNH2twpELsYf3glHLlOhnnlIMovIeEgEmcjgyo=","1G0+8546Y6jCIUXG0cKJq0qpkd6NsnG+4w9oSVW3gH8="]})";
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPost;

  EXPECT_EQ(url_request, expected_url_request);
}

}  // namespace ads
