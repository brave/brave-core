/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"

namespace rewards_browsertest {

class Request {
 public:
  Request(const std::string& url, int32_t method) : url(url), method(method) {}
  std::string url;
  int32_t method;
};

class RewardsBrowserTestResponse {
 public:
  RewardsBrowserTestResponse();
  ~RewardsBrowserTestResponse();

  void LoadMocks();

  void Get(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response);

  std::vector<Request> GetRequests();

  void ClearRequests();

  void SetSKUOrder(ledger::SKUOrderPtr order);

  void SetPromotionEmptyKey(const bool empty);

  void SetAlternativePublisherList(const bool alternative);

  std::string GetUpholdAddress();

  void SetVerifiedWallet(const bool verified);

  void SetExternalBalance(const std::string& balance);

  void SetUserFundsBalance(const bool user_funds);

 private:
  std::string registrar_vk_;
  std::string verification_;
  std::string promotions_;
  std::string promotion_empty_key_;
  std::string promotion_claim_;
  std::string creds_tokens_;
  std::string creds_tokens_prod_;
  std::string creds_tokens_sku_;
  std::string creds_tokens_sku_prod_;
  std::string captcha_;
  std::string balance_;
  std::string parameters_;
  std::string uphold_auth_resp_;
  std::string uphold_transactions_resp_;
  std::string uphold_commit_resp_;
  std::string user_funds_balance_resp_;

  std::vector<Request> requests_;
  bool empty_promotion_key_ = false;
  bool alternative_publisher_list_ = false;
  ledger::SKUOrderPtr order_;
  bool verified_wallet_ = false;
  std::string external_balance_ = "0.0";
  bool user_funds_balance_ = false;
  std::map<std::string, std::string> publisher_prefixes_;
};

}  // namespace rewards_browsertest
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_
