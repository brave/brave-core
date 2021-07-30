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

namespace ledger {
namespace endpoint {
namespace promotion {

PutCaptcha::PutCaptcha(LedgerImpl* ledger):
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

type::Result PutCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid solution");
    return type::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to verify captcha solution");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
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

  auto request = type::UrlRequest::New();
  request->url = GetUrl(captcha_id);
  request->content = GeneratePayload(x, y);
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::PUT;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PutCaptcha::OnRequest(
    const type::UrlResponse& response,
    PutCaptchaCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
