/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/post_transaction_gemini/post_transaction_sku_gemini.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::payment {

PostTransactionGemini::PostTransactionGemini(RewardsEngine& engine)
    : engine_(engine) {}

PostTransactionGemini::~PostTransactionGemini() = default;

std::string PostTransactionGemini::GetUrl(const std::string& order_id) {
  return engine_->Get<EnvironmentConfig>()
      .rewards_payment_url()
      .Resolve(base::StrCat({"/v1/orders/", order_id, "/transactions/gemini"}))
      .spec();
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

void PostTransactionGemini::Request(const mojom::SKUTransaction& transaction,
                                    PostTransactionGeminiCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(transaction.order_id);
  request->content = GeneratePayload(transaction);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->Log(FROM_HERE) << "External Transaction ID: "
                          << transaction.external_transaction_id << " for "
                          << transaction.amount;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostTransactionGemini::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostTransactionGemini::OnRequest(PostTransactionGeminiCallback callback,
                                      mojom::UrlResponsePtr response) {
  DCHECK(response);

  if (CheckStatusCode(response->status_code) != mojom::Result::OK) {
    engine_->Log(FROM_HERE)
        << "Error creating gemini transaction on the payment server";
  }

  std::move(callback).Run(CheckStatusCode(response->status_code));
}

}  // namespace brave_rewards::internal::endpoint::payment
