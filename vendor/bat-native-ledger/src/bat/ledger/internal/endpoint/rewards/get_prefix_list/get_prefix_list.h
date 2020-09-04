/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_
#define BRAVELEDGER_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /publishers/prefix-list
//
// Success code:
// HTTP_OK (200)
//
// Response body:
// blob

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace rewards {

using GetPrefixListCallback = std::function<void(
    const type::Result result,
    const std::string& body)>;

class GetPrefixList {
 public:
  explicit GetPrefixList(LedgerImpl* ledger);
  ~GetPrefixList();

  void Request(GetPrefixListCallback callback);

 private:
  std::string GetUrl();

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const type::UrlResponse& response,
      GetPrefixListCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace rewards
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_REWARDS_GET_PREFIX_LIST_GET_PREFIX_LIST_H_
