/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_API_GET_PARAMETERS_GET_PARAMETERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_API_GET_PARAMETERS_GET_PARAMETERS_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v1/parameters
// GET /v1/parameters?currency={currency}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "batRate": 0.2476573499489187,
//   "autocontribute": {
//     "choices": [
//       5,
//       10,
//       15,
//       20,
//       25,
//       50,
//       100
//     ],
//     "defaultChoice": 20
//   },
//   "tips": {
//     "defaultTipChoices": [
//       1,
//       10,
//       100
//     ],
//     "defaultMonthlyChoices": [
//       1,
//       10,
//       100
//     ]
//   },
//   "payoutStatus": {
//     "unverified": "off",
//     "uphold": "off",
//     "gemini": "off",
//     "bitflyer": "complete"
//   }
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace api {

using GetParametersCallback =
    base::OnceCallback<void(type::Result result,
                            const type::RewardsParameters&)>;

class GetParameters {
 public:
  explicit GetParameters(LedgerImpl* ledger);
  ~GetParameters();

  void Request(GetParametersCallback callback);

 private:
  std::string GetUrl(const std::string& currency = "");

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      type::RewardsParameters* parameters);

  void OnRequest(GetParametersCallback callback,
                 const type::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace api
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_API_GET_PARAMETERS_GET_PARAMETERS_H_
