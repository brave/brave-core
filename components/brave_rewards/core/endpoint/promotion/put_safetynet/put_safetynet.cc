/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/put_safetynet/put_safetynet.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PutSafetynet::PutSafetynet(RewardsEngineImpl& engine) : engine_(engine) {}

PutSafetynet::~PutSafetynet() = default;

std::string PutSafetynet::GetUrl(const std::string& nonce) {
  const std::string path =
      base::StringPrintf("/v2/attestations/safetynet/%s", nonce.c_str());

  return GetServerUrl(path);
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
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void PutSafetynet::OnRequest(PutSafetynetCallback callback,
                             mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
