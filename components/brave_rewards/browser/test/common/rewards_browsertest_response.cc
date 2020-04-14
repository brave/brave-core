/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_split.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/test/base/ui_test_utils.h"

namespace rewards_browsertest {

using braveledger_request_util::ServerTypes;
using rewards_browsertest_util::URLMatches;

RewardsBrowserTestResponse::RewardsBrowserTestResponse() = default;

RewardsBrowserTestResponse::~RewardsBrowserTestResponse() = default;

void RewardsBrowserTestResponse::LoadMocks() {
  base::FilePath path;
  rewards_browsertest_util::GetTestDataDir(&path);
  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("register_persona_resp.json"),
      &registrar_vk_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("verify_persona_resp.json"),
      &verification_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("promotions_resp.json"),
      &promotions_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("promotion_empty_key_resp.json"),
      &promotion_empty_key_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("captcha_resp.json"),
      &captcha_));
  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("promotion_claim_resp.json"),
      &promotion_claim_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("creds_tokens_resp.json"),
      &creds_tokens_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("creds_tokens_prod_resp.json"),
      &creds_tokens_prod_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("parameters_resp.json"),
      &parameters_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("balance_resp.json"),
      &balance_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_auth_resp.json"),
      &uphold_auth_resp_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_transactions_resp.json"),
      &uphold_transactions_resp_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_commit_resp.json"),
      &uphold_commit_resp_));
}

void RewardsBrowserTestResponse::Get(
    const std::string& url,
    int32_t method,
    int* response_status_code,
    std::string* response) {
  requests_.emplace_back(url, method);
  DCHECK(response_status_code && response);

  std::vector<std::string> tmp = base::SplitString(
      url,
      "/",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_ALL);
  const std::string persona_url =
      braveledger_request_util::BuildUrl(REGISTER_PERSONA, PREFIX_V2);
  if (url.find(persona_url) == 0 && tmp.size() == 6) {
    *response = registrar_vk_;
    return;
  }

  if (URLMatches(
      url,
      REGISTER_PERSONA,
      PREFIX_V2,
      ServerTypes::LEDGER) && tmp.size() == 7) {
    *response = verification_;
    return;
  }

  if (URLMatches(
      url,
      "/wallet/",
      PREFIX_V2,
      ServerTypes::BALANCE)) {
    *response = balance_;
    return;
  }

  if (URLMatches(
      url,
      "/parameters",
      PREFIX_V1,
      ServerTypes::kAPI)) {
    *response = parameters_;
    return;
  }

  if (URLMatches(
      url,
      "/promotions?",
      PREFIX_V1,
      ServerTypes::kPromotion)) {
    if (empty_promotion_key_) {
      *response = promotion_empty_key_;
    } else {
      *response = promotions_;
    }
    return;
  }

  if (URLMatches(
      url,
      "/promotions/",
      PREFIX_V1,
      ServerTypes::kPromotion)) {
    if (url.find("claims") != std::string::npos) {
      *response = creds_tokens_;
    } else {
      *response = promotion_claim_;
    }
    return;
  }

  if (URLMatches(
      url,
      "/captchas",
      PREFIX_V1,
      ServerTypes::kPromotion)) {
    *response = captcha_;
  }

  if (URLMatches(
      url,
      "/api/v3/public/channels",
      "",
      ServerTypes::kPublisher)) {
    if (alternative_publisher_list_) {
      *response =
          "["
          "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
          "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}],"
          "[\"laurenwags.github.io\",\"wallet_connected\",false,\"address2\","
            "{\"donationAmounts\": [5,10,20]}]"
          "]";
    } else {
      *response =
          "["
          "[\"bumpsmack.com\",\"publisher_verified\",false,\"address1\",{}],"
          "[\"duckduckgo.com\",\"wallet_connected\",false,\"address2\",{}],"
          "[\"3zsistemi.si\",\"wallet_connected\",false,\"address3\",{}],"
          "[\"site1.com\",\"wallet_connected\",false,\"address4\",{}],"
          "[\"site2.com\",\"wallet_connected\",false,\"address5\",{}],"
          "[\"site3.com\",\"wallet_connected\",false,\"address6\",{}],"
          "[\"laurenwags.github.io\",\"wallet_connected\",false,\"address2\","
            "{\"donationAmounts\": [5,10,20]}],"
          "[\"kjozwiakstaging.github.io\",\"wallet_connected\",false,\"aa\","
            "{\"donationAmounts\": [5,50,100]}]"
          "]";
    }

    // we only have one page in this mock, so next page should return end
    if (url.find("page=2") != std::string::npos) {
      *response_status_code = net::HTTP_NO_CONTENT;
    }

    return;
  }

  if (base::StartsWith(
      url,
      braveledger_uphold::GetAPIUrl("/oauth2/token"),
      base::CompareCase::INSENSITIVE_ASCII)) {
    *response = uphold_auth_resp_;
    return;
  }

  if (base::StartsWith(
      url,
      braveledger_uphold::GetAPIUrl("/v0/me/cards"),
      base::CompareCase::INSENSITIVE_ASCII)) {
    if (base::EndsWith(
        url,
        "transactions",
        base::CompareCase::INSENSITIVE_ASCII)) {
      *response = uphold_transactions_resp_;
      *response_status_code = net::HTTP_ACCEPTED;
    } else if (base::EndsWith(
        url,
        "commit",
        base::CompareCase::INSENSITIVE_ASCII)) {
      *response = uphold_commit_resp_;
    } else {
      *response = rewards_browsertest_util::GetUpholdCard(
          external_balance_,
          rewards_browsertest_util::GetUpholdExternalAddress());
    }
    return;
  }

  if (base::StartsWith(
      url,
      braveledger_uphold::GetAPIUrl("/v0/me"),
      base::CompareCase::INSENSITIVE_ASCII)) {
    *response = rewards_browsertest_util::GetUpholdUser(verified_wallet_);
    return;
  }

  if (URLMatches(
      url,
      "/order",
      PREFIX_V1,
      ServerTypes::kPayments)) {
    if (url.find("credentials") != std::string::npos) {
      if (method == 0) {
        #if defined(OFFICIAL_BUILD)
          *response = creds_tokens_prod_;
        #else
          *response = creds_tokens_;
        #endif

        return;
      }
      return;
    } else if (url.find("transaction") == std::string::npos) {
      *response = rewards_browsertest_util::GetOrderCreateResponse(
          order_->Clone());
    }

    *response_status_code = net::HTTP_CREATED;
    return;
  }
}

std::vector<Request> RewardsBrowserTestResponse::GetRequests() {
  return requests_;
}

void RewardsBrowserTestResponse::ClearRequests() {
  requests_.clear();
}

void RewardsBrowserTestResponse::SetSKUOrder(ledger::SKUOrderPtr order) {
  order_ = std::move(order);
}

void RewardsBrowserTestResponse::SetPromotionEmptyKey(bool empty) {
  empty_promotion_key_ = empty;
}

void RewardsBrowserTestResponse::SetAlternativePublisherList(bool alternative) {
  alternative_publisher_list_ = alternative;
}

void RewardsBrowserTestResponse::SetVerifiedWallet(const bool verified) {
  verified_wallet_ = verified;
}

void RewardsBrowserTestResponse::SetExternalBalance(
    const std::string& balance) {
  external_balance_ = balance;
}

}  // namespace rewards_browsertest
