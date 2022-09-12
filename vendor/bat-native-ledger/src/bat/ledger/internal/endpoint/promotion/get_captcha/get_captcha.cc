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

namespace ledger {
namespace endpoint {
namespace promotion {

GetCaptcha::GetCaptcha(LedgerImpl* ledger):
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

mojom::Result GetCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid captcha id");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized captcha id");
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to generate the captcha image");
    return mojom::Result::LEDGER_ERROR;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result GetCaptcha::ParseBody(const std::string& body,
                                    std::string* image) {
  DCHECK(image);

  std::string encoded_image;
  base::Base64Encode(body, &encoded_image);
  *image =
      base::StringPrintf("data:image/jpeg;base64,%s", encoded_image.c_str());

  return mojom::Result::LEDGER_OK;
}

void GetCaptcha::Request(
    const std::string& captcha_id,
    GetCaptchaCallback callback) {
  auto url_callback = base::BindOnce(
      &GetCaptcha::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(captcha_id);
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetCaptcha::OnRequest(GetCaptchaCallback callback,
                           const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response, true);

  std::string image;
  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, image);
    return;
  }

  result = ParseBody(response.body, &image);
  std::move(callback).Run(result, image);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
