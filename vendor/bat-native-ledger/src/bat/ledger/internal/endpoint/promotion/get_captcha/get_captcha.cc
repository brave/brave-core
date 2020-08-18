/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/get_captcha/get_captcha.h"

#include <utility>

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

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
namespace endpoint {
namespace promotion {

GetCaptcha::GetCaptcha(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetCaptcha::~GetCaptcha() = default;

std::string GetCaptcha::GetUrl(const std::string& captcha_id) {
  const std::string path = base::StringPrintf(
      "/v1/captchas/%s.png",
      captcha_id.c_str());

  return GetServerUrl(path);
}

ledger::Result GetCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid captcha id");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized captcha id");
    return ledger::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to generate the captcha image");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetCaptcha::ParseBody(
    const std::string& body,
    std::string* image) {
  DCHECK(image);

  std::string encoded_image;
  base::Base64Encode(body, &encoded_image);
  *image =
      base::StringPrintf("data:image/jpeg;base64,%s", encoded_image.c_str());

  return ledger::Result::LEDGER_OK;
}

void GetCaptcha::Request(
    const std::string& captcha_id,
    GetCaptchaCallback callback) {
  auto url_callback = std::bind(&GetCaptcha::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(captcha_id),
      {},
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetCaptcha::OnRequest(
    const ledger::UrlResponse& response,
    GetCaptchaCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

  std::string image;
  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, image);
    return;
  }

  result = ParseBody(response.body, &image);
  callback(result, image);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
