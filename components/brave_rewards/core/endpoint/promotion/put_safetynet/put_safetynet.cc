/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_safetynet/put_safetynet.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PutSafetynet::PutSafetynet(RewardsEngineImpl& engine) : engine_(engine) {}

PutSafetynet::~PutSafetynet() = default;

std::string PutSafetynet::GetUrl(const std::string& nonce) {
  auto url =
      URLHelpers::Resolve(engine_->Get<EnvironmentConfig>().rewards_grant_url(),
                          {"/v2/attestations/safetynet/", nonce});
  return url.spec();
}

std::string PutSafetynet::GeneratePayload(const std::string& token) {
  base::Value::Dict dictionary;
  dictionary.Set("token", token);
  std::string payload;
  base::JSONWriter::Write(dictionary, &payload);

  return payload;
}

mojom::Result PutSafetynet::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    engine_->LogError(FROM_HERE) << "Invalid request";
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    engine_->LogError(FROM_HERE) << "Invalid solution";
    return mojom::Result::CAPTCHA_FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    engine_->LogError(FROM_HERE) << "Failed to verify captcha solution";
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PutSafetynet::Request(const std::string& token,
                           const std::string& nonce,
                           PutSafetynetCallback callback) {
  auto url_callback = base::BindOnce(
      &PutSafetynet::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(nonce);
  request->content = GeneratePayload(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::PUT;

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PutSafetynet::OnRequest(PutSafetynetCallback callback,
                             mojom::UrlResponsePtr response) {
  DCHECK(response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
