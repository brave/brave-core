/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_DEVICECHECK_POST_DEVICECHECK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_DEVICECHECK_POST_DEVICECHECK_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v1/devicecheck/attestations
//
// Request body:
// {
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
//   "publicKeyHash": "f3f2f3ffqdwfqwfwqfd"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
//
// Response body:
// {
//   "nonce": "c4645786-052f-402f-8593-56af2f7a21ce"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PostDevicecheckCallback =
    base::OnceCallback<void(mojom::Result result, const std::string& nonce)>;

class PostDevicecheck {
 public:
  explicit PostDevicecheck(LedgerImpl* ledger);
  ~PostDevicecheck();

  void Request(const std::string& key, PostDevicecheckCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload(const std::string& key);

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body, std::string* nonce);

  void OnRequest(PostDevicecheckCallback callback,
                 const mojom::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_DEVICECHECK_POST_DEVICECHECK_H_
