/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"

namespace brave_rewards::test_util {

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

  void SetSKUOrder(mojom::SKUOrderPtr order);

  void SetAlternativePublisherList(const bool alternative);

  void SetExternalBalance(const std::string& balance);

 private:
  std::string wallet_;
  std::string creds_tokens_sku_;
  std::string creds_tokens_sku_prod_;
  std::string parameters_;
  std::string uphold_auth_resp_;
  std::string uphold_transactions_resp_;
  std::string uphold_commit_resp_;

  std::vector<Request> requests_;
  bool alternative_publisher_list_ = false;
  mojom::SKUOrderPtr order_;
  std::string external_balance_ = "0.0";
  std::map<std::string, std::string> publisher_prefixes_;
};

}  // namespace brave_rewards::test_util

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_RESPONSE_H_
