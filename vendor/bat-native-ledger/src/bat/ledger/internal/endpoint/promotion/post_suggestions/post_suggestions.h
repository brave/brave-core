/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_POST_SUGGESTIONS_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_POST_SUGGESTIONS_H_

#include <string>

#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/ledger.h"

// POST /v1/suggestions
//
// Request body:
// {
//   "credentials": [
//     {
//       "t": "",
//       "publicKey": "",
//       "signature": ""
//     }
//   ],
//   "suggestion": "base64_string"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_SERVICE_UNAVAILABLE (503)
//
// Response body:
// {Empty}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PostSuggestionsCallback = std::function<void(
    const type::Result result)>;

class PostSuggestions {
 public:
  explicit PostSuggestions(LedgerImpl* ledger);
  ~PostSuggestions();

  void Request(
      const credential::CredentialsRedeem& redeem,
      PostSuggestionsCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(
      const credential::CredentialsRedeem& redeem);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const type::UrlResponse& response,
      PostSuggestionsCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_POST_SUGGESTIONS_H_
