/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_PUT_SAFETYNET_PUT_SAFETYNET_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_PUT_SAFETYNET_PUT_SAFETYNET_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PutSafetynetCallback = std::function<void(const ledger::Result result)>;

class PutSafetynet {
 public:
  explicit PutSafetynet(bat_ledger::LedgerImpl* ledger);
  ~PutSafetynet();

  void Request(
      const std::string& token,
      const std::string& nonce,
      PutSafetynetCallback callback);

 private:
  std::string GetUrl(const std::string& nonce);

  std::string GeneratePayload(const std::string& token);

  ledger::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const ledger::UrlResponse& response,
      PutSafetynetCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_PUT_SAFETYNET_PUT_SAFETYNET_H_
