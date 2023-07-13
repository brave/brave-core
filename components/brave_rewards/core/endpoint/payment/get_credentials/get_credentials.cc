/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/payment/get_credentials/get_credentials.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/payment/payment_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace payment {

GetCredentials::GetCredentials(RewardsEngineImpl& engine) : engine_(engine) {}

GetCredentials::~GetCredentials() = default;

std::string GetCredentials::GetUrl(const std::string& order_id,
                                   const std::string& item_id) {
  const std::string path = base::StringPrintf(
      "/v1/orders/%s/credentials/%s", order_id.c_str(), item_id.c_str());

  return GetServerUrl(path);
}

mojom::Result GetCredentials::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::RETRY;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return mojom::Result::RETRY;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::RETRY;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::RETRY;
  }

  return mojom::Result::OK;
}

mojom::Result GetCredentials::ParseBody(const std::string& body,
                                        mojom::CredsBatch* batch) {
  DCHECK(batch);
  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::RETRY;
  }

  const base::Value::Dict& dict = value->GetDict();
  auto* batch_proof = dict.FindString("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return mojom::Result::RETRY;
  }

  auto* signed_creds = dict.FindList("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return mojom::Result::RETRY;
  }

  auto* public_key = dict.FindString("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
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
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void GetCredentials::OnRequest(GetCredentialsCallback callback,
                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  auto batch = mojom::CredsBatch::New();
  result = ParseBody(response->body, batch.get());
  std::move(callback).Run(result, std::move(batch));
}

}  // namespace payment
}  // namespace endpoint
}  // namespace brave_rewards::internal
