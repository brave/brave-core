/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_

#include <string>

#include "bat/ledger/ledger.h"

// POST /v1/captchas
//
// Request body:
// {
//   "paymentId": "83b3b77b-e7c3-455b-adda-e476fa0656d2"
// }
//
// Success code:
// HTTP_OK (200)
//
// Error codes:
// HTTP_BAD_REQUEST (400)
//
// Response body:
// {
//   "hint": "circle",
//   "captchaId": "d155d2d2-2627-425b-9be8-44ae9f541762"
// }

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using PostCaptchaCallback =
    base::OnceCallback<void(mojom::Result result,
                            const std::string& hint,
                            const std::string& captcha_id)>;

class PostCaptcha {
 public:
  explicit PostCaptcha(LedgerImpl* ledger);
  ~PostCaptcha();

  void Request(PostCaptchaCallback callback);

 private:
  std::string GetUrl();

  std::string GeneratePayload();

  mojom::Result CheckStatusCode(const int status_code);

  mojom::Result ParseBody(const std::string& body,
                          std::string* hint,
                          std::string* captcha_id);

  void OnRequest(PostCaptchaCallback callback,
                 const mojom::UrlResponse& response);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_PROMOTION_POST_CAPTCHA_POST_CAPTCHA_H_
