/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_VOTES_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_VOTES_ENDPOINT_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/internal/core/url_fetcher.h"
#include "bat/ledger/internal/payments/payment_data.h"

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

class PostVotesEndpoint : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "payments-post-votes-endpoint";

  URLRequest MapRequest(const std::string& publisher_id,
                        PaymentVoteType vote_type,
                        const std::vector<PaymentVote>& votes);

  bool MapResponse(const URLResponse& response);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PAYMENTS_POST_VOTES_ENDPOINT_H_
