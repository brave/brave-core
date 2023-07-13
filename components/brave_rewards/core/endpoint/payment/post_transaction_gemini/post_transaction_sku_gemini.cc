/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/payment/payment_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

PostTransactionGemini::PostTransactionGemini(RewardsEngineImpl& engine)
    : engine_(engine) {}

PostTransactionGemini::~PostTransactionGemini() = default;

std::string PostTransactionGemini::GetUrl(const std::string& order_id) {
  const std::string path =
      base::StringPrintf("/v1/orders/%s/transactions/gemini", order_id.c_str());

  return GetServerUrl(path);
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
  auto url_callback =
      std::bind(&PostTransactionGemini::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  BLOG(0, "External Transaction ID: " << transaction.external_transaction_id
                                      << " for " << transaction.amount);

  engine_->LoadURL(std::move(request), url_callback);
}

void PostTransactionGemini::OnRequest(mojom::UrlResponsePtr response,
                                      PostTransactionGeminiCallback callback) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  BLOG_IF(0, CheckStatusCode(response->status_code) != mojom::Result::OK,
          "Error creating gemini transaction on the payment server");
  BLOG_IF(0, CheckStatusCode(response->status_code) == mojom::Result::OK,
          "Gemini transaction successful on the payment server");

  callback(CheckStatusCode(response->status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
