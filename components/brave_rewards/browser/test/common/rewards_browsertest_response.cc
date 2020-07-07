/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/big_endian.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "bat/ledger/internal/publisher/prefix_util.h"
#include "bat/ledger/internal/publisher/protos/channel_response.pb.h"
#include "bat/ledger/internal/publisher/protos/publisher_prefix_list.pb.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/test/base/ui_test_utils.h"

namespace {

std::string GetPublisherPrefixListResponse(
    const std::map<std::string, std::string>& prefix_map) {
  std::string prefixes;
  for (const auto& pair : prefix_map) {
    prefixes += pair.first;
  }

  publishers_pb::PublisherPrefixList message;
  message.set_prefix_size(4);
  message.set_compression_type(
      publishers_pb::PublisherPrefixList::NO_COMPRESSION);
  message.set_uncompressed_size(prefixes.size());
  message.set_prefixes(std::move(prefixes));

  std::string out;
  message.SerializeToString(&out);
  return out;
}

void AddUpholdWalletToChannelResponse(
    publishers_pb::ChannelResponse* response,
    std::string address,
    publishers_pb::UpholdWalletState wallet_state =
        publishers_pb::UPHOLD_ACCOUNT_KYC) {
  auto* uphold_wallet = response->add_wallets()->mutable_uphold_wallet();
  uphold_wallet->set_wallet_state(wallet_state);
  uphold_wallet->set_address(address);
}

std::string GetPublisherChannelResponse(
    const std::map<std::string, std::string>& prefix_map,
    const std::string& prefix,
    bool use_alternate_publisher_list) {
  std::string key;
  for (const auto& pair : prefix_map) {
    std::string hex = base::ToLowerASCII(
        base::HexEncode(pair.first.data(), pair.first.size()));
    if (hex.find(prefix) == 0) {
      key = pair.second;
      break;
    }
  }

  if (key.empty()) {
    return "";
  }

  publishers_pb::ChannelResponseList message;
  auto* channel = message.add_channel_responses();
  bool maybe_hide = false;

  channel->set_channel_identifier(key);

  if (key == "bumpsmack.com") {
    AddUpholdWalletToChannelResponse(
        channel,
        "address1",
        publishers_pb::UPHOLD_ACCOUNT_NO_KYC);
  } else if (key == "duckduckgo.com") {
    AddUpholdWalletToChannelResponse(channel, "address2");
  } else if (key == "3zsistemi.si") {
    maybe_hide = true;
    AddUpholdWalletToChannelResponse(channel, "address3");
  } else if (key == "site1.com") {
    maybe_hide = true;
    AddUpholdWalletToChannelResponse(channel, "address4");
  } else if (key == "site2.com") {
    maybe_hide = true;
    AddUpholdWalletToChannelResponse(channel, "address5");
  } else if (key == "site3.com") {
    maybe_hide = true;
    AddUpholdWalletToChannelResponse(channel, "address6");
  } else if (key == "laurenwags.github.io") {
    AddUpholdWalletToChannelResponse(channel, "address2");
    auto* banner = channel->mutable_site_banner_details();
    banner->add_donation_amounts(5);
    banner->add_donation_amounts(10);
    banner->add_donation_amounts(20);
  } else if (key == "kjozwiakstaging.github.io") {
    maybe_hide = true;
    AddUpholdWalletToChannelResponse(channel, "aa");
    auto* banner = channel->mutable_site_banner_details();
    banner->add_donation_amounts(5);
    banner->add_donation_amounts(50);
    banner->add_donation_amounts(100);
  }

  if (maybe_hide && use_alternate_publisher_list) {
    return "";
  }

  std::string out;
  message.SerializeToString(&out);

  // Add padding header
  uint32_t length = out.length();
  out.insert(0, 4, ' ');
  base::WriteBigEndian(&out[0], length);
  return out;
}

}  // namespace

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
      path.AppendASCII("creds_tokens_sku_resp.json"),
      &creds_tokens_sku_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("creds_tokens_sku_prod_resp.json"),
      &creds_tokens_sku_prod_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("parameters_resp.json"),
      &parameters_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("balance_resp.json"),
      &balance_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("user_funds_balance_resp.json"),
      &user_funds_balance_resp_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_auth_resp.json"),
      &uphold_auth_resp_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_transactions_resp.json"),
      &uphold_transactions_resp_));

  ASSERT_TRUE(base::ReadFileToString(
      path.AppendASCII("uphold_commit_resp.json"),
      &uphold_commit_resp_));

  std::vector<std::string> publisher_keys {
      "bumpsmack.com",
      "duckduckgo.com",
      "3zsistemi.si",
      "site1.com",
      "site2.com",
      "site3.com",
      "laurenwags.github.io",
      "kjozwiakstaging.github.io"
  };

  for (auto& key : publisher_keys) {
    std::string prefix = braveledger_publisher::GetHashPrefixRaw(key, 4);
    publisher_prefixes_[prefix] = key;
  }
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
    if (user_funds_balance_) {
      *response = user_funds_balance_resp_;
    } else {
      *response = balance_;
    }
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
      "/publishers/prefix-list",
      "",
      ServerTypes::kPublisher)) {
    *response = GetPublisherPrefixListResponse(publisher_prefixes_);
    }

  if (URLMatches(
      url,
      "/publishers/prefixes/",
      "",
      ServerTypes::kPrivateCDN)) {
    size_t start = url.rfind('/') + 1;
    if (start < url.length()) {
      *response = GetPublisherChannelResponse(
          publisher_prefixes_,
          url.substr(start),
          alternative_publisher_list_);
    } else {
      *response = "";
    }
    if (response->empty()) {
      *response_status_code = net::HTTP_NOT_FOUND;
    }
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
          *response = creds_tokens_sku_prod_;
        #else
          *response = creds_tokens_sku_;
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

void RewardsBrowserTestResponse::SetUserFundsBalance(const bool user_funds) {
  user_funds_balance_ = user_funds;
}

}  // namespace rewards_browsertest
