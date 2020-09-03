/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_
#define BRAVELEDGER_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_

#include <string>

#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/ledger.h"

// POST /v1/votes
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
//   "vote": "base64_string"
// }
//
// Success code:
// HTTP_CREATED (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {Empty}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace payment {

using PostVotesCallback = std::function<void(const type::Result result)>;

class PostVotes {
 public:
  explicit PostVotes(LedgerImpl* ledger);
  ~PostVotes();

  void Request(
      const credential::CredentialsRedeem& redeem,
      PostVotesCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(
      const credential::CredentialsRedeem& redeem);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(
      const type::UrlResponse& response,
      PostVotesCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace payment
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PAYMENT_POST_VOTES_POST_VOTES_H_
