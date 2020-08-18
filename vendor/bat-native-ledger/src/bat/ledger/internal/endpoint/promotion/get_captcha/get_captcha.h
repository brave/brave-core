/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
#define BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace ledger {
namespace endpoint {
namespace promotion {

using GetCaptchaCallback = std::function<void(
    const ledger::Result result,
    const std::string& image)>;

class GetCaptcha {
 public:
  explicit GetCaptcha(bat_ledger::LedgerImpl* ledger);
  ~GetCaptcha();

  void Request(
    const std::string& captcha_id,
    GetCaptchaCallback callback);

 private:
  std::string GetUrl(const std::string& captcha_id);

  ledger::Result CheckStatusCode(const int status_code);

  ledger::Result ParseBody(
      const std::string& body,
      std::string* image);

  void OnRequest(
      const ledger::UrlResponse& response,
      GetCaptchaCallback callback);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVELEDGER_ENDPOINT_PROMOTION_GET_CAPTCHA_GET_CAPTCHA_H_
