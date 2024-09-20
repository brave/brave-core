/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_uphold/post_transaction_uphold.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::payment {

PostTransactionUphold::PostTransactionUphold(RewardsEngine& engine)
    : engine_(engine) {}

PostTransactionUphold::~PostTransactionUphold() = default;

std::string PostTransactionUphold::GetUrl(const std::string& order_id) {
  return engine_->Get<EnvironmentConfig>()
      .rewards_payment_url()
      .Resolve(base::StrCat({"/v1/orders/", order_id, "/transactions/uphold"}))
      .spec();
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
    engine_->LogError(FROM_HERE) << "Invalid request";
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Unrecognized transaction suffix";
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_CONFLICT) {
    engine_->LogError(FROM_HERE) << "External transaction id already submitted";
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    engine_->LogError(FROM_HERE) << "Internal server error";
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_CREATED && status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostTransactionUphold::Request(const mojom::SKUTransaction& transaction,
                                    PostTransactionUpholdCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostTransactionUphold::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostTransactionUphold::OnRequest(PostTransactionUpholdCallback callback,
                                      mojom::UrlResponsePtr response) {
  DCHECK(response);
  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace brave_rewards::internal::endpoint::payment
