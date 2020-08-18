/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_captcha/post_captcha.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

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
namespace endpoint {
namespace promotion {

PostCaptcha::PostCaptcha(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostCaptcha::~PostCaptcha() = default;

std::string PostCaptcha::GetUrl() {
  return GetServerUrl("/v1/captchas");
}

std::string PostCaptcha::GeneratePayload() {
  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey(
      "paymentId",
      ledger_->state()->GetPaymentId());

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

ledger::Result PostCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostCaptcha::ParseBody(
    const std::string& body,
    std::string* hint,
    std::string* captcha_id) {
  DCHECK(hint && captcha_id);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* captcha_id_parse = dictionary->FindStringKey("captchaId");
  if (!captcha_id_parse) {
    BLOG(0, "Captcha id is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* hint_parse = dictionary->FindStringKey("hint");
  if (!hint_parse) {
    BLOG(0, "Hint is wrong");
    return ledger::Result::LEDGER_ERROR;
  }

  *captcha_id = *captcha_id_parse;
  *hint = *hint_parse;

  return ledger::Result::LEDGER_OK;
}

void PostCaptcha::Request(PostCaptchaCallback callback) {
  auto url_callback = std::bind(&PostCaptcha::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      {},
      GeneratePayload(),
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostCaptcha::OnRequest(
    const ledger::UrlResponse& response,
    PostCaptchaCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  std::string hint;
  std::string captcha_id;
  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, hint, captcha_id);
    return;
  }

  result = ParseBody(response.body, &hint, &captcha_id);
  callback(result, hint, captcha_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
