/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_

#include <memory>
#include <string>

#include "base/values.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PostCredsCallback = std::function<void(
    const ledger::Result result,
    const std::string& claim_id)>;

class PostCreds {
 public:
  explicit PostCreds(bat_ledger::LedgerImpl* ledger);
  ~PostCreds();

  void Request(
    const std::string& promotion_id,
    std::unique_ptr<base::ListValue> blinded_creds,
    PostCredsCallback callback);

 private:
  std::string GetUrl(const std::string& promotion_id);

  std::string GeneratePayload(std::unique_ptr<base::ListValue> blinded_creds);

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      std::string* claim_id);

  void OnRequest(
      const ledger::UrlResponse& response,
      PostCredsCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_CREDS_POST_CREDS_H_
