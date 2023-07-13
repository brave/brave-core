/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_captcha/put_captcha.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PutCaptcha::PutCaptcha(RewardsEngineImpl& engine) : engine_(engine) {}

PutCaptcha::~PutCaptcha() = default;

std::string PutCaptcha::GetUrl(const std::string& captcha_id) {
  const std::string path =
      base::StringPrintf("/v1/captchas/%s", captcha_id.c_str());

  return GetServerUrl(path);
}

std::string PutCaptcha::GeneratePayload(const int x, const int y) {
  base::Value::Dict dictionary;
  base::Value::Dict solution_dict;
  solution_dict.Set("x", x);
  solution_dict.Set("y", y);
  dictionary.Set("solution", std::move(solution_dict));
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  return payload;
}

mojom::Result PutCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid solution");
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Failed to verify captcha solution");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PutCaptcha::Request(const int x,
                         const int y,
                         const std::string& captcha_id,
                         PutCaptchaCallback callback) {
  auto url_callback = base::BindOnce(
      &PutCaptcha::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(captcha_id);
  request->content = GeneratePayload(x, y);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::PUT;
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void PutCaptcha::OnRequest(PutCaptchaCallback callback,
                           mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
