/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_

#include <string>

#include "bat/ledger/ledger.h"

// GET /v1/captchas/{captcha_id}.png
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
// {PNG data}

namespace ledger {
class LedgerImpl;

namespace endpoint {
namespace promotion {

using GetCaptchaCallback = std::function<void(
    const type::Result result,
    const std::string& image)>;

class GetCaptcha {
 public:
  explicit GetCaptcha(LedgerImpl* ledger);
  ~GetCaptcha();

  void Request(
    const std::string& captcha_id,
    GetCaptchaCallback callback);

 private:
  std::string GetUrl(const std::string& captcha_id);

  type::Result CheckStatusCode(const int status_code);

  type::Result ParseBody(
      const std::string& body,
      std::string* image);

  void OnRequest(
      const type::UrlResponse& response,
      GetCaptchaCallback callback);

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
