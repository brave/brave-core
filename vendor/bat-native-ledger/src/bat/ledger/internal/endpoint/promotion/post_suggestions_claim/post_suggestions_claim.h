/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_\
POST_SUGGESTIONS_CLAIM_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_\
POST_SUGGESTIONS_CLAIM_H_

#include <string>

#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using PostSuggestionsClaimCallback = std::function<void(
    const ledger::Result result)>;

class PostSuggestionsClaim {
 public:
  explicit PostSuggestionsClaim(bat_ledger::LedgerImpl* ledger);
  ~PostSuggestionsClaim();

  void Request(
      const braveledger_credentials::CredentialsRedeem& redeem,
      PostSuggestionsClaimCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(
      const braveledger_credentials::CredentialsRedeem& redeem);

  ledger::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const ledger::UrlResponse& response,
      PostSuggestionsClaimCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_\
// POST_SUGGESTIONS_CLAIM_H_
