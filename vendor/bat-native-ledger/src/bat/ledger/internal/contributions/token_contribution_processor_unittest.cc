/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contributions/token_contribution_processor.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "bat/ledger/internal/contributions/contribution_token_manager.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/core/sql_store.h"
#include "net/http/http_status_code.h"

namespace ledger {

static const char kPublicKey[] = "6AphTvx13IgxVRG1nljV2ql1Y7yGUol6yrVMhEP85wI=";

// clang-format off
static const std::array<std::string, 5> kUnblindedTokens = {
  "iUiBvs6tjgvTEONwcNEXSNfYdDXww96tXpL98uyKZGl6MKXjMiF/FpZC8U4cAQmxmcB2g3EFEo6JdFcak2nd/4w+CrHgtmvbcEkaaoMyIPDlEy6q3rktXILDdx0SfHR4",  // NOLINT
  "8sHNxIecEcEdXrvEFFH+2N81JbJ4oCi+aTO7M9SNB4BPyn4CNGg1PeQcSiE5MCiaMNtK6LoypuofvgKda++iTjSQHWcRKFHTiJAzcAOBXc/129fcBJ1kHUKX/WLgRV9U",  // NOLINT
  "wpHj2VimK6gBFmlt6Dn+6IDnS4zKlbXB4ij7VzzDq8r419RMIiufL3dm6slyJ2rMBLc5zhigRT5CSv1SiqExFU50NlugiRLzkocVJk48ZZaOF6CqNS2c69M0ZjLQIZwq",  // NOLINT
  "lrdAqfW4lvhpHI2Wn9by4jGl8yMPpTBGCGkYu9fJV1T9UxhXvDiyinTtcBxhOuVT/XOHMe0YkCr/MphNkxZWqzwKdzhEW4Yt2j2o5k4kh78ueuBCPmD4ysV2p/Mizcxd",  // NOLINT
  "UbSA3UGMaIct8FrIkeQw0/RYxikfktX1xPbDLf0gqBnovnMNpTyGBveOeE5dJJPP59ijPkALAUjWOvwJXhPIPcS86KKY8N9KJp0LVPcbELiUnjopOYoP3VqRSfNkQKB4"  // NOLINT
};
// clang-format on

class TokenContributionProcessorTest : public BATLedgerTest {
 protected:
  void InsertTokens(ContributionTokenType token_type) {
    std::vector<ContributionToken> tokens;
    for (auto& unblinded_token : kUnblindedTokens) {
      tokens.push_back({.id = 0,
                        .value = 0.25,
                        .unblinded_token = unblinded_token,
                        .public_key = kPublicKey});
    }
    context().Get<ContributionTokenManager>().InsertTokens(tokens, token_type);
  }

  void InsertPublisher(bool registered) {
    static const char kSQL[] = R"sql(
      INSERT INTO server_publisher_info
        (publisher_key, status, address, updated_at)
      VALUES ('brave.com', ?, '', ?)
    )sql";

    context().Get<SQLStore>().Run(kSQL, registered ? 1 : 0,
                                  base::Time::Now().ToDoubleT());
  }

  TokenContributionResult ProcessContribution(ContributionSource source) {
    return WaitFor(
        context().Get<TokenContributionProcessor>().ProcessContribution(
            MakeContribution(source)));
  }

  Contribution MakeContribution(ContributionSource source) {
    return {.type = ContributionType::kOneTime,
            .publisher_id = "brave.com",
            .amount = 1.0,
            .source = source};
  }
};

TEST_F(TokenContributionProcessorTest, InsufficientTokens) {
  InitializeLedger();

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveVG),
            TokenContributionResult::kInsufficientTokens);
}

TEST_F(TokenContributionProcessorTest, PublisherNotFound) {
  InitializeLedger();
  InsertTokens(ContributionTokenType::kVG);

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveVG),
            TokenContributionResult::kPublisherNotRegistered);

  InsertPublisher(false);

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveVG),
            TokenContributionResult::kPublisherNotRegistered);
}

TEST_F(TokenContributionProcessorTest, VgEndpointError) {
  InitializeLedger();
  InsertTokens(ContributionTokenType::kVG);
  InsertPublisher(true);

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveVG),
            TokenContributionResult::kRedeemError);
}

TEST_F(TokenContributionProcessorTest, VgSuccess) {
  InitializeLedger();
  InsertTokens(ContributionTokenType::kVG);
  InsertPublisher(true);

  auto response = mojom::UrlResponse::New();
  response->status_code = net::HTTP_OK;

  AddNetworkResultForTesting("https://grant.rewards.brave.com/v1/suggestions",
                             mojom::UrlMethod::POST, std::move(response));

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveVG),
            TokenContributionResult::kSuccess);
}

TEST_F(TokenContributionProcessorTest, SkuEndpointError) {
  InitializeLedger();
  InsertTokens(ContributionTokenType::kSKU);
  InsertPublisher(true);

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveSKU),
            TokenContributionResult::kRedeemError);
}

TEST_F(TokenContributionProcessorTest, SkuSuccess) {
  InitializeLedger();
  InsertTokens(ContributionTokenType::kSKU);
  InsertPublisher(true);

  auto response = mojom::UrlResponse::New();
  response->status_code = net::HTTP_OK;

  AddNetworkResultForTesting("https://payment.rewards.brave.com/v1/votes",
                             mojom::UrlMethod::POST, std::move(response));

  EXPECT_EQ(ProcessContribution(ContributionSource::kBraveSKU),
            TokenContributionResult::kSuccess);
}

}  // namespace ledger
