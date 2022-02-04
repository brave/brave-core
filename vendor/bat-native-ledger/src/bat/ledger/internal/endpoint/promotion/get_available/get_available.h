/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_

#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

// GET /v1/promotions?migrate=true&paymentId={payment_id}&platform={platform}
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body:
// {
//   "promotions": [
//     {
//       "id": "83b3b77b-e7c3-455b-adda-e476fa0656d2",
//       "createdAt": "2020-06-08T15:04:45.352584Z",
//       "expiresAt": "2020-10-08T15:04:45.352584Z",
//       "version": 5,
//       "suggestionsPerGrant": 120,
//       "approximateValue": "30",
//       "type": "ugp",
//       "available": true,
//       "platform": "desktop",
//       "publicKeys": [
//         "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
//       ],
//       "legacyClaimed": false,
//       "claimableUntil": "2020-10-08T15:04:45.352584Z"
//     }
//   ]
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using GetAvailableCallback = std::function<void(
    const type::Result result,
    type::PromotionList list,
    const std::vector<std::string>& corrupted_promotions)>;

class GetAvailable {
 public:
  explicit GetAvailable(LedgerImpl* ledger);
  ~GetAvailable();

  void Request(
    const std::string& platform,
    GetAvailableCallback callback);

 private:
  std::string GetUrl(const std::string& platform);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      type::PromotionList* list,
      std::vector<std::string>* corrupted_promotions);

  void OnRequest(
      const type::UrlResponse& response,
      GetAvailableCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_GET_AVAILABLE_GET_AVAILABLE_H_
