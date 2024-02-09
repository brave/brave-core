/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostClobberedClaims::PostClobberedClaims(RewardsEngineImpl& engine)
    : engine_(engine) {}

PostClobberedClaims::~PostClobberedClaims() = default;

std::string PostClobberedClaims::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve("/v2/promotions/reportclobberedclaims")
      .spec();
}

std::string PostClobberedClaims::GeneratePayload(
    base::Value::List corrupted_claims) {
  base::Value::Dict body;
  body.Set("claimIds", std::move(corrupted_claims));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostClobberedClaims::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    engine_->LogError(FROM_HERE) << "Invalid request";
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    engine_->LogError(FROM_HERE) << "Internal server error";
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostClobberedClaims::Request(base::Value::List corrupted_claims,
                                  PostClobberedClaimsCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(std::move(corrupted_claims));
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostClobberedClaims::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostClobberedClaims::OnRequest(PostClobberedClaimsCallback callback,
                                    mojom::UrlResponsePtr response) {
  DCHECK(response);
  callback(CheckStatusCode(response->status_code));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
