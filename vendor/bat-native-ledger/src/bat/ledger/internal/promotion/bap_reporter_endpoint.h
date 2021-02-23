/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_ENDPOINT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_ENDPOINT_H_

#include <string>

#include "base/values.h"
#include "bat/ledger/ledger.h"

// POST /v1/promotions/report-bap
//
// Request body:
// {
//   "amount": <number>
// }
//
// Success code:
// HTTP_OK (200)
//
// Error Codes:
// HTTP_BAD_REQUEST (400)
// HTTP_CONFLICT (409) Endpoint already called for this payment ID
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "reportBapId": <uuidv4>
// }

namespace ledger {

class LedgerImpl;

namespace promotion {

class BAPReporterEndpoint {
 public:
  explicit BAPReporterEndpoint(LedgerImpl* ledger);
  ~BAPReporterEndpoint();

  using Callback = std::function<void(bool)>;
  void Request(double amount, Callback callback);

 private:
  void OnFetchCompleted(Callback callback, const type::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_BAP_REPORTER_ENDPOINT_H_
