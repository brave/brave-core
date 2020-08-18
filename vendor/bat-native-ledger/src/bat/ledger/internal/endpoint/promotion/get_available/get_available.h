/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_

#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using GetAvailableCallback = std::function<void(
    const ledger::Result result,
    ledger::PromotionList list,
    const std::vector<std::string>& corrupted_promotions)>;

class GetAvailable {
 public:
  explicit GetAvailable(bat_ledger::LedgerImpl* ledger);
  ~GetAvailable();

  void Request(
    const std::string& platform,
    GetAvailableCallback callback);

 private:
  std::string GetUrl(const std::string& platform);

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      ledger::PromotionList* list,
      std::vector<std::string>* corrupted_promotions);

  void OnRequest(
      const ledger::UrlResponse& response,
      GetAvailableCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_
