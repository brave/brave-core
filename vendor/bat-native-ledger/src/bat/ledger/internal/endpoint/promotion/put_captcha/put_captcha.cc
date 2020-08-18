/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/put_captcha/put_captcha.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

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
namespace endpoint {
namespace promotion {

PutCaptcha::PutCaptcha(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PutCaptcha::~PutCaptcha() = default;

std::string PutCaptcha::GetUrl(const std::string& captcha_id) {
  const std::string path = base::StringPrintf(
      "/v1/captchas/%s",
      captcha_id.c_str());

  return GetServerUrl(path);
}

std::string PutCaptcha::GeneratePayload(const int x, const int y) {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  base::Value solution_dict(base::Value::Type::DICTIONARY);
  solution_dict.SetIntKey("x", x);
  solution_dict.SetIntKey("y", y);
  dictionary.SetKey("solution", std::move(solution_dict));
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  return payload;
}

ledger::Result PutCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid solution");
    return ledger::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to verify captcha solution");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

void PutCaptcha::Request(
      const int x,
      const int y,
      const std::string& captcha_id,
      PutCaptchaCallback callback) {
  auto url_callback = std::bind(&PutCaptcha::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(captcha_id),
      {},
      GeneratePayload(x, y),
      "application/json; charset=utf-8",
      ledger::UrlMethod::PUT,
      url_callback);
}

void PutCaptcha::OnRequest(
    const ledger::UrlResponse& response,
    PutCaptchaCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
