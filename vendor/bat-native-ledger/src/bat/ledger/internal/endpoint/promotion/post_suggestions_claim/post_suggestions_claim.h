// NOLINT(build/header_guard)
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

// POST /v1/suggestions/claim
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
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
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

using PostSuggestionsClaimCallback =
    std::function<void(const type::Result result)>;

using PostSuggestionsClaimCallbackV2 =
    std::function<void(const type::Result result, std::string drain_id)>;

class PostSuggestionsClaim {
 public:
  explicit PostSuggestionsClaim(LedgerImpl* ledger);
  ~PostSuggestionsClaim();

  void Request(const credential::CredentialsRedeem& redeem,
               PostSuggestionsClaimCallback callback);

  void RequestV2(const credential::CredentialsRedeem& redeem,
                 PostSuggestionsClaimCallbackV2 callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const credential::CredentialsRedeem& redeem);

  type::Result CheckStatusCode(const int status_code);

  mojo::StructPtr<type::UrlRequest> GetSuggestionRequest(
      const type::BraveWalletPtr wallet,
      const char* path,
      const std::string& payload);

  void OnRequest(const type::UrlResponse& response,
                 PostSuggestionsClaimCallback callback);

  void OnRequestV2(const type::UrlResponse& response,
                   PostSuggestionsClaimCallbackV2 callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_POST_SUGGESTIONS_CLAIM_\
// POST_SUGGESTIONS_CLAIM_H_
