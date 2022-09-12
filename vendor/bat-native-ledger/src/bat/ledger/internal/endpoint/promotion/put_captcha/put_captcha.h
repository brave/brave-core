/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_

#include <string>

#include "bat/ledger/ledger.h"

// PUT /v1/captchas/{captcha_id}
//
// Request body:
// {
//   "solution": {
//     "x": 10,
//     "y": 50
//   }
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
// HTTP_UNAUTHORIZED (401)
// HTTP_INTERNAL_SERVER_ERROR (500)
//
// Response Format (success):
// {Empty}
//
// Response Format (error):
// {
//   "message": "Error solving captcha",
//   "code": 401
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PutCaptchaCallback = base::OnceCallback<void(mojom::Result)>;

class PutCaptcha {
 public:
  explicit PutCaptcha(LedgerImpl* ledger);
  ~PutCaptcha();

  void Request(
      const int x,
      const int y,
      const std::string& captcha_id,
      PutCaptchaCallback callback);

 private:
  std::string GetUrl(const std::string& captcha_id);

  std::string GeneratePayload(const int x, const int y);

  mojom::Result CheckStatusCode(const int status_code);

  void OnRequest(PutCaptchaCallback callback,
                 const mojom::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_PUT_CAPTCHA_PUT_CAPTCHA_H_
