/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_credentials.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/logging/logging.h"
#include "net/http/http_status_code.h"

namespace braveledger_response_util {

// Request Url:
// GET /v1/promotions/{promotion_id}/claims/{claim_id}
// GET /v1/orders/{order_id}/credentials
// GET /v1/orders/{order_id}/credentials/{item_path}
//
// Success:
// OK (200)
//
// Response Format (success):
// {
//   "id": "9c9aed7f-b349-452e-80a8-95faf2b1600d",
//   "orderId": "f2e6494e-fb21-44d1-90e9-b5408799acd8",
//   "issuerId": "138bf9ca-69fe-4540-9ac4-bc65baddc4a0",
//   "signedCreds": [
//     "ijSZoLLG+EnRN916RUQcjiV6c4Wb6ItbnxXBFhz81EQ=",
//     "dj6glCJ2roHYcTFcXF21IrKx1uT/ptM7SJEdiEE1fG8=",
//     "nCF9a4KuASICVC0zrx2wGnllgIUxBMnylpu5SA+oBjI=",
//   ],
//   "batchProof": "zx0cdJhaB/OdYcUtnyXdi+lsoniN2vRTZ1w0U4D7Mgeu1I7RwB+tYKNgFU",
//   "publicKey": "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
// }
//
// Response Format (error):
// {
//   "message": "Claim has been accepted but is not ready",
//   "code": 202,
//   "data": {}
// }

ledger::Result ParseSignedCreds(
    const ledger::UrlResponse& response,
    base::Value* result) {
  DCHECK(result);

  // Accepted (202)
  if (response.status_code == net::HTTP_ACCEPTED) {
    return ledger::Result::RETRY_SHORT;
  }

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  // Not Found (404)
  if (response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return ledger::Result::NOT_FOUND;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::LEDGER_ERROR;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* batch_proof = dictionary->FindStringKey("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* signed_creds = dictionary->FindListKey("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return ledger::Result::LEDGER_ERROR;
  }

  auto* public_key = dictionary->FindStringKey("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return ledger::Result::LEDGER_ERROR;
  }

  result->SetStringKey("batch_proof", *batch_proof);
  result->SetStringKey("public_key", *public_key);
  result->SetKey("signed_creds", base::Value(signed_creds->GetList()));

  return ledger::Result::LEDGER_OK;
}

}  // namespace braveledger_response_util
