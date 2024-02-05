/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_credentials/post_credentials.h"

#include <utility>

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

PostCredentials::PostCredentials(RewardsEngineImpl& engine) : engine_(engine) {}

PostCredentials::~PostCredentials() = default;

std::string PostCredentials::GetUrl(const std::string& order_id) {
  auto url = URLHelpers::Resolve(
      engine_->Get<EnvironmentConfig>().rewards_payment_url(),
      {"/v1/orders/", order_id, "/credentials"});
  return url.spec();
}

std::string PostCredentials::GeneratePayload(
    const std::string& item_id,
    const std::string& type,
    base::Value::List&& blinded_creds) {
  base::Value::Dict body;
  body.Set("itemId", item_id);
  body.Set("type", type);
  body.Set("blindedCreds", std::move(blinded_creds));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Credentials already exist for this order");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostCredentials::Request(const std::string& order_id,
                              const std::string& item_id,
                              const std::string& type,
                              base::Value::List&& blinded_creds,
                              PostCredentialsCallback callback) {
  auto url_callback = base::BindOnce(
      &PostCredentials::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(order_id);
  request->content = GeneratePayload(item_id, type, std::move(blinded_creds));
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PostCredentials::OnRequest(PostCredentialsCallback callback,
                                mojom::UrlResponsePtr response) {
  DCHECK(response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
