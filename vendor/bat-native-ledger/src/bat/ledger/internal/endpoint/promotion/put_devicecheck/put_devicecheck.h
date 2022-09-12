/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_DEVICECHECK_PUT_DEVICECHECK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_DEVICECHECK_PUT_DEVICECHECK_H_

#include <string>

#include "bat/ledger/ledger.h"

// PUT /v1/devicecheck/attestations/{nonce}
//
// Request body:
// {
//   "attestationBlob": "dfasdfasdpflsadfplf2r23re2",
//   "signature": "435dfasdfaadff34f43sdpflsadfplf2r23re2"
// }
//
// Success:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response body (success):
// {Empty}
//
// Response body (error):
// {
//   "message": "Error solving captcha",
//   "code": 401
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PutDevicecheckCallback = base::OnceCallback<void(mojom::Result)>;

class PutDevicecheck {
 public:
  explicit PutDevicecheck(LedgerImpl* ledger);
  ~PutDevicecheck();

  void Request(
      const std::string& blob,
      const std::string& signature,
      const std::string& nonce,
      PutDevicecheckCallback callback);

 private:
  std::string GetUrl(const std::string& nonce);

  std::string GeneratePayload(
      const std::string& blob,
      const std::string& signature);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PutDevicecheckCallback callback,
                 const mojom::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_DEVICECHECK_PUT_DEVICECHECK_H_
