/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"

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

PostTransactionGemini::PostTransactionGemini(RewardsEngineImpl& engine)
    : engine_(engine) {}

PostTransactionGemini::~PostTransactionGemini() = default;

std::string PostTransactionGemini::GetUrl(const std::string& order_id) {
  auto url = URLHelpers::Resolve(
      engine_->Get<EnvironmentConfig>().rewards_payment_url(),
      {"/v1/orders/", order_id, "/transactions/gemini"});
  return url.spec();
}

std::string PostTransactionGemini::GeneratePayload(
    const mojom::SKUTransaction& transaction) {
  base::Value::Dict body;
  body.Set("externalTransactionId", transaction.external_transaction_id);
  body.Set("kind", "gemini");

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

mojom::Result PostTransactionGemini::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized transaction suffix");
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "External transaction id already submitted");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_CREATED && status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostTransactionGemini::Request(const mojom::SKUTransaction& transaction,
                                    PostTransactionGeminiCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  BLOG(0, "External Transaction ID: " << transaction.external_transaction_id
                                      << " for " << transaction.amount);

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostTransactionGemini::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostTransactionGemini::OnRequest(PostTransactionGeminiCallback callback,
                                      mojom::UrlResponsePtr response) {
  DCHECK(response);

  BLOG_IF(0, CheckStatusCode(response->status_code) != mojom::Result::OK,
          "Error creating gemini transaction on the payment server");
  BLOG_IF(0, CheckStatusCode(response->status_code) == mojom::Result::OK,
          "Gemini transaction successful on the payment server");

  callback(CheckStatusCode(response->status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
