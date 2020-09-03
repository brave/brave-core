/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_GET_SIGNED_CREDS_GET_SIGNED_CREDS_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_GET_SIGNED_CREDS_GET_SIGNED_CREDS_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v1/promotions/{promotion_id}/claims/{claim_id}
//
// Success code:
// HTTP_OK (200)
//
// Error Codes:
// HTTP_ACCEPTED (202)
// HTTP_BAD_REQUEST (400)
// HTTP_NOT_FOUND (404)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body (success):
// {
//   "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
//   "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "issuerId": "138bf9ca-69fe-4540-9ac4-bc65baddc4a0",
//   "signedCreds": [
//     "ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=",
//     "dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=",
//     "nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI="
//   ],
//   "batchProof": "zx0cdJhaB/OdYcUtnyXdi+lsoniN2vRTZ1w0U4D7Mgeu1I7RwB+tYKNgFU",
//   "publicKey": "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
// }
//
// Response body (error):
// {
//   "message": "Claim has been accepted but is not ready",
//   "code": 202,
//   "data": {}
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using GetSignedCredsCallback = std::function<void(
    const type::Result result,
    type::CredsBatchPtr batch)>;

class GetSignedCreds {
 public:
  explicit GetSignedCreds(LedgerImpl* ledger);
  ~GetSignedCreds();

  void Request(
    const std::string& promotion_id,
    const std::string& claim_id,
    GetSignedCredsCallback callback);

 private:
  std::string GetUrl(
    const std::string& promotion_id,
    const std::string& claim_id);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      type::CredsBatch* batch);

  void OnRequest(
      const type::UrlResponse& response,
      GetSignedCredsCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_GET_SIGNED_CREDS_GET_SIGNED_CREDS_H_
