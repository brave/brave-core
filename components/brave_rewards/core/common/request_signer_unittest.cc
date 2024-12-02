/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/request_signer.h"

#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter='RewardsRequestSignerTest.*'

namespace brave_rewards::internal {

class RewardsRequestSignerTest : public testing::Test {
 protected:
  void SetUp() override {
    auto seed =
        base::Base64Decode("WLw9wU40/9Kgjzl4ZO7+GftbszIU82dR5bQotwV4K5Y=");
    DCHECK(seed);

    rewards_wallet_.payment_id = "a1ca63f7-e2be-5f4c-9c01-2fec88f08c28";
    rewards_wallet_.recovery_seed = std::move(*seed);
  }

  mojom::RewardsWallet rewards_wallet_;
};

TEST_F(RewardsRequestSignerTest, GetDigest) {
  auto seed = Signer::GenerateRecoverySeed();
  auto digest = RequestSigner::GetDigest(
      base::byte_span_with_nul_from_cstring("hello world"));
  EXPECT_EQ(digest, "SHA-256=QwZGhH5wNEwJ9Yc56Z1byW6sjV/nKVzxlrmGJ5h2v5s=");
}

TEST_F(RewardsRequestSignerTest, FromRewardsWallet) {
  mojom::RewardsWallet invalid_rewards_wallet;
  EXPECT_FALSE(RequestSigner::FromRewardsWallet(invalid_rewards_wallet));
  auto signer = RequestSigner::FromRewardsWallet(rewards_wallet_);
  ASSERT_TRUE(signer);
  EXPECT_EQ(signer->key_id(), rewards_wallet_.payment_id);
  EXPECT_EQ(base::Base64Encode(signer->signer().secret_key()),
            "KJLbC4ns7fyEz+0FKo8mCD3oZ9ENCfqDsSnRjSujjpT1+"
            "q6bYxO6nga8XZD86NqxnaXjgJUsAK+Re3xofvdLwA==");
}

TEST_F(RewardsRequestSignerTest, SignRequest) {
  auto signer = RequestSigner::FromRewardsWallet(rewards_wallet_);
  ASSERT_TRUE(signer);

  // Signing should fail for an empty request.
  mojom::UrlRequest request;
  EXPECT_FALSE(signer->SignRequest(request));

  // Signing should fail if the URL is invalid.
  request.url = "invalid_url";
  EXPECT_FALSE(signer->SignRequest(request));

  request.url = "https://example.com/v1/api/path";
  request.method = mojom::UrlMethod::GET;
  request.content = "hello world";

  // Correct headers should be added.
  EXPECT_TRUE(signer->SignRequest(request));

  ASSERT_EQ(request.headers.size(), static_cast<size_t>(3));
  EXPECT_EQ(request.headers[0],
            "digest: SHA-256=uU0nuZNNPgilLlLX2n2r+sSE7+N6U4DukIj3rOLvzek=");
  EXPECT_EQ(request.headers[1],
            "signature: keyId=\"a1ca63f7-e2be-5f4c-9c01-2fec88f08c28\","
            "algorithm=\"ed25519\",headers=\"digest (request-target)\","
            "signature=\"joOwvl4m4pHnZb599tmoD3Ov3vM6CGhN3lBcIzCLoyxnMnYaQq8dpi"
            "RZ84/uHgTSHJlRHiloM3ubxQjHTWE2BQ==\"");
  EXPECT_EQ(request.headers[2], "accept: application/json");

  // An alternative key ID should also work.
  request.headers.clear();
  signer->set_key_id("arbitrary_key");

  EXPECT_TRUE(signer->SignRequest(request));

  ASSERT_EQ(request.headers.size(), static_cast<size_t>(3));
  EXPECT_EQ(request.headers[0],
            "digest: SHA-256=uU0nuZNNPgilLlLX2n2r+sSE7+N6U4DukIj3rOLvzek=");
  EXPECT_EQ(request.headers[1],
            "signature: keyId=\"arbitrary_key\",algorithm=\"ed25519\","
            "headers=\"digest (request-target)\","
            "signature=\"joOwvl4m4pHnZb599tmoD3Ov3vM6CGhN3lBcIzCLoyxnMnYaQq8dpi"
            "RZ84/uHgTSHJlRHiloM3ubxQjHTWE2BQ==\"");
  EXPECT_EQ(request.headers[2], "accept: application/json");
}

TEST_F(RewardsRequestSignerTest, GetSignedHeaders) {
  auto signer = RequestSigner::FromRewardsWallet(rewards_wallet_);
  ASSERT_TRUE(signer);

  auto headers = signer->GetSignedHeaders("get /api/path", "hello world");

  ASSERT_EQ(headers.size(), static_cast<size_t>(3));
  EXPECT_EQ(headers[0],
            "digest: SHA-256=uU0nuZNNPgilLlLX2n2r+sSE7+N6U4DukIj3rOLvzek=");
  EXPECT_EQ(headers[1],
            "signature: keyId=\"a1ca63f7-e2be-5f4c-9c01-2fec88f08c28\","
            "algorithm=\"ed25519\",headers=\"digest (request-target)\","
            "signature=\"Kot6Cg1j9CyljYZgJHxkONLIVVeTw8me05TtBnF3MeJvHSWQlGv0ak"
            "jUuSprBvazFS+P1IBj9SAO1xwmnriyCQ==\"");
  EXPECT_EQ(headers[2], "accept: application/json");
}

TEST_F(RewardsRequestSignerTest, SignHeaders) {
  auto signer = RequestSigner::FromRewardsWallet(rewards_wallet_);
  ASSERT_TRUE(signer);

  std::vector<std::pair<std::string, std::string>> headers{
      {"header_a", "value_a"}, {"header_b", "value_b"}};

  EXPECT_EQ(signer->SignHeaders(headers),
            "keyId=\"a1ca63f7-e2be-5f4c-9c01-2fec88f08c28\","
            "algorithm=\"ed25519\",headers=\"header_a header_b\","
            "signature=\"XTsQG7Aiwi6lGZGySwIrMg6+iNxEMyO0+iN/HZfzGRIhrdcWFZpkpZ"
            "CRHCluTgfhQsyk25SrK0UjajMZ988JAw==\"");
}

}  // namespace brave_rewards::internal
