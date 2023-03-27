/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger.h"

// GET /publishers/prefix-list
//
// Success code:
// HTTP_OK (200)
//
// Response body:
// blob

namespace brave_rewards::core {
class LedgerImpl;

namespace endpoint {
namespace rewards {

using GetPrefixListCallback =
    std::function<void(const mojom::Result result, const std::string& body)>;

class GetPrefixList {
 public:
  explicit GetPrefixList(LedgerImpl* ledger);
  ~GetPrefixList();

  void Request(GetPrefixListCallback callback);

 private:
  std::string GetUrl();

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(const mojom::UrlResponse& response,
                 GetPrefixListCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace rewards
}  // namespace endpoint
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_
