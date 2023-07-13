/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"

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

PostTransactionUphold::PostTransactionUphold(RewardsEngineImpl& engine)
    : engine_(engine) {}

PostTransactionUphold::~PostTransactionUphold() = default;

std::string PostTransactionUphold::GetUrl(const std::string& order_id) {
  const std::string path =
      base::StringPrintf("/v1/orders/%s/transactions/uphold", order_id.c_str());

  return GetServerUrl(path);
}

std::string PostTransactionUphold::GeneratePayload(
    const mojom::SKUTransaction& transaction) {
  base::Value::Dict body;
  body.Set("externalTransactionId", transaction.external_transaction_id);
  body.Set("kind", "uphold");

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

mojom::Result PostTransactionUphold::CheckStatusCode(const int status_code) {
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

void PostTransactionUphold::Request(const mojom::SKUTransaction& transaction,
                                    PostTransactionUpholdCallback callback) {
  auto url_callback =
      std::bind(&PostTransactionUphold::OnRequest, this, _1, callback);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->LoadURL(std::move(request), url_callback);
}

void PostTransactionUphold::OnRequest(mojom::UrlResponsePtr response,
                                      PostTransactionUpholdCallback callback) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);
  callback(CheckStatusCode(response->status_code));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
