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

namespace ledger {
namespace endpoint {
namespace promotion {

PostCaptcha::PostCaptcha(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostCaptcha::~PostCaptcha() = default;

std::string PostCaptcha::GetUrl() {
  return GetServerUrl("/v1/captchas");
}

std::string PostCaptcha::GeneratePayload() {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value::Dict body;
  body.Set("paymentId", wallet->payment_id);

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result PostCaptcha::ParseBody(const std::string& body,
                                     std::string* hint,
                                     std::string* captcha_id) {
  DCHECK(hint && captcha_id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* captcha_id_parse = dict.FindString("captchaId");
  if (!captcha_id_parse) {
    BLOG(0, "Captcha id is wrong");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* hint_parse = dict.FindString("hint");
  if (!hint_parse) {
    BLOG(0, "Hint is wrong");
    return mojom::Result::LEDGER_ERROR;
  }

  *captcha_id = *captcha_id_parse;
  *hint = *hint_parse;

  return mojom::Result::LEDGER_OK;
}

void PostCaptcha::Request(PostCaptchaCallback callback) {
  auto url_callback = base::BindOnce(
      &PostCaptcha::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void PostCaptcha::OnRequest(PostCaptchaCallback callback,
                            const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  std::string hint;
  std::string captcha_id;
  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, hint, captcha_id);
    return;
  }

  result = ParseBody(response.body, &hint, &captcha_id);
  std::move(callback).Run(result, hint, captcha_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
