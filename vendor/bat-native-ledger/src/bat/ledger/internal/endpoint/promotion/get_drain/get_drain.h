/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_DRAIN_GET_DRAIN_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_DRAIN_GET_DRAIN_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v1/promotions/drain/<drain id>
//
// Request body:
// {Empty}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST  (400)
// HTTP_NOT_FOUND    (404)
// HTTP_SERVER_ERROR (500)
//
// Response body:
// {
//     “drainId”: <drain id>,   // uuidv4
//     “status”: <status enum>, // Status enum -> (“pending”, “in-progress”,
//     “delayed”, “complete”)
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

class GetDrain {
 public:
  explicit GetDrain(LedgerImpl* ledger);
  ~GetDrain();

  void Request(const std::string& drain_id, GetDrainCallback callback);

 private:
  std::string GetUrl(const std::string& drain_id);

  type::Result CheckStatusCode(const int status_code);

  void OnRequest(const type::UrlResponse& response, GetDrainCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_DRAIN_GET_DRAIN_H_
