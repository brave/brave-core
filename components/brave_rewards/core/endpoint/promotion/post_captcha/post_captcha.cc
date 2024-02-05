/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/post_captcha/post_captcha.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostCaptcha::PostCaptcha(RewardsEngineImpl& engine) : engine_(engine) {}

PostCaptcha::~PostCaptcha() = default;

std::string PostCaptcha::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve("/v1/captchas")
      .spec();
}

std::string PostCaptcha::GeneratePayload() {
  const auto wallet = engine_->wallet()->GetWallet();
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
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result PostCaptcha::ParseBody(const std::string& body,
                                     std::string* hint,
                                     std::string* captcha_id) {
  DCHECK(hint && captcha_id);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* captcha_id_parse = dict.FindString("captchaId");
  if (!captcha_id_parse) {
    BLOG(0, "Captcha id is wrong");
    return mojom::Result::FAILED;
  }

  const auto* hint_parse = dict.FindString("hint");
  if (!hint_parse) {
    BLOG(0, "Hint is wrong");
    return mojom::Result::FAILED;
  }

  *captcha_id = *captcha_id_parse;
  *hint = *hint_parse;

  return mojom::Result::OK;
}

void PostCaptcha::Request(PostCaptchaCallback callback) {
  auto url_callback = base::BindOnce(
      &PostCaptcha::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PostCaptcha::OnRequest(PostCaptchaCallback callback,
                            mojom::UrlResponsePtr response) {
  DCHECK(response);

  std::string hint;
  std::string captcha_id;
  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, hint, captcha_id);
    return;
  }

  result = ParseBody(response->body, &hint, &captcha_id);
  std::move(callback).Run(result, hint, captcha_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
