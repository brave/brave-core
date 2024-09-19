/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::payment {

GetCredentials::GetCredentials(RewardsEngine& engine) : engine_(engine) {}

GetCredentials::~GetCredentials() = default;

std::string GetCredentials::GetUrl(const std::string& order_id,
                                   const std::string& item_id) {
  return engine_->Get<EnvironmentConfig>()
      .rewards_payment_url()
      .Resolve(
          base::StrCat({"/v1/orders/", order_id, "/credentials/", item_id}))
      .spec();
}

mojom::Result GetCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    engine_->LogError(FROM_HERE) << "Invalid request";
    return mojom::Result::RETRY;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    engine_->LogError(FROM_HERE) << "Unrecognized claim id";
    return mojom::Result::RETRY;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    engine_->LogError(FROM_HERE) << "Internal server error";
    return mojom::Result::RETRY;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::RETRY;
  }

  return mojom::Result::OK;
}

mojom::Result GetCredentials::ParseBody(const std::string& body,
                                        mojom::CredsBatch* batch) {
  DCHECK(batch);
  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::RETRY;
  }

  const base::Value::Dict& dict = value->GetDict();
  auto* batch_proof = dict.FindString("batchProof");
  if (!batch_proof) {
    engine_->LogError(FROM_HERE) << "Missing batch proof";
    return mojom::Result::RETRY;
  }

  auto* signed_creds = dict.FindList("signedCreds");
  if (!signed_creds) {
    engine_->LogError(FROM_HERE) << "Missing signed creds";
    return mojom::Result::RETRY;
  }

  auto* public_key = dict.FindString("publicKey");
  if (!public_key) {
    engine_->LogError(FROM_HERE) << "Missing public key";
    return mojom::Result::RETRY;
  }

  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;
  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);

  return mojom::Result::OK;
}

void GetCredentials::Request(const std::string& order_id,
                             const std::string& item_id,
                             GetCredentialsCallback callback) {
  auto url_callback = base::BindOnce(
      &GetCredentials::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(order_id, item_id);

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void GetCredentials::OnRequest(GetCredentialsCallback callback,
                               mojom::UrlResponsePtr response) {
  DCHECK(response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  auto batch = mojom::CredsBatch::New();
  result = ParseBody(response->body, batch.get());
  std::move(callback).Run(result, std::move(batch));
}

}  // namespace brave_rewards::internal::endpoint::payment
