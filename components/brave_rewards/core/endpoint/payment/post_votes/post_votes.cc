/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_votes/post_votes.h"

#include <utility>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

PostVotes::PostVotes(RewardsEngineImpl& engine) : engine_(engine) {}

PostVotes::~PostVotes() = default;

std::string PostVotes::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .rewards_payment_url()
      .Resolve("/v1/votes")
      .spec();
}

std::string PostVotes::GeneratePayload(
    const credential::CredentialsRedeem& redeem) {
  base::Value::Dict data;
  data.Set("type", credential::ConvertRewardTypeToString(redeem.type));
  if (!redeem.order_id.empty()) {
    data.Set("orderId", redeem.order_id);
  }
  data.Set("channel", redeem.publisher_key);

  std::string data_json;
  base::JSONWriter::Write(data, &data_json);
  std::string data_encoded;
  base::Base64Encode(data_json, &data_encoded);

  base::Value::List credentials =
      credential::GenerateCredentials(redeem.token_list, data_encoded);

  base::Value::Dict payload;
  payload.Set("vote", data_encoded);
  payload.Set("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PostVotes::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostVotes::Request(const credential::CredentialsRedeem& redeem,
                        PostVotesCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload(redeem);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostVotes::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostVotes::OnRequest(PostVotesCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);
  callback(CheckStatusCode(response->status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
