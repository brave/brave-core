/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/get_captcha/get_captcha.h"

#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

GetCaptcha::GetCaptcha(RewardsEngineImpl& engine) : engine_(engine) {}

GetCaptcha::~GetCaptcha() = default;

std::string GetCaptcha::GetUrl(const std::string& captcha_id) {
  auto url =
      URLHelpers::Resolve(engine_->Get<EnvironmentConfig>().rewards_grant_url(),
                          {"/v1/captchas/", captcha_id, ".png"});
  return url.spec();
}

mojom::Result GetCaptcha::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    engine_->LogError(FROM_HERE) << "Invalid captcha id";
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Unrecognized captcha id";
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    engine_->LogError(FROM_HERE) << "Failed to generate the captcha image";
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetCaptcha::ParseBody(const std::string& body,
                                    std::string* image) {
  DCHECK(image);

  std::string encoded_image;
  base::Base64Encode(body, &encoded_image);
  *image = "data:image/jpeg;base64," + encoded_image;

  return mojom::Result::OK;
}

void GetCaptcha::Request(const std::string& captcha_id,
                         GetCaptchaCallback callback) {
  auto url_callback = base::BindOnce(
      &GetCaptcha::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(captcha_id);
  engine_->Get<URLLoader>().Load(std::move(request), URLLoader::LogLevel::kNone,
                                 std::move(url_callback));
}

void GetCaptcha::OnRequest(GetCaptchaCallback callback,
                           mojom::UrlResponsePtr response) {
  DCHECK(response);

  std::string image;
  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, image);
    return;
  }

  result = ParseBody(response->body, &image);
  std::move(callback).Run(result, image);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
