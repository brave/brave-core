/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/get_signed_creds/get_signed_creds.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

GetSignedCreds::GetSignedCreds(RewardsEngineImpl& engine) : engine_(engine) {}

GetSignedCreds::~GetSignedCreds() = default;

std::string GetSignedCreds::GetUrl(const std::string& promotion_id,
                                   const std::string& claim_id) {
  const std::string& path = base::StringPrintf(
      "/v1/promotions/%s/claims/%s", promotion_id.c_str(), claim_id.c_str());

  return GetServerUrl(path);
}

mojom::Result GetSignedCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_ACCEPTED) {
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized claim id");
    return mojom::Result::NOT_FOUND;
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

mojom::Result GetSignedCreds::ParseBody(const std::string& body,
                                        mojom::CredsBatch* batch) {
  DCHECK(batch);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* batch_proof = dict.FindString("batchProof");
  if (!batch_proof) {
    BLOG(0, "Missing batch proof");
    return mojom::Result::FAILED;
  }

  auto* signed_creds = dict.FindList("signedCreds");
  if (!signed_creds) {
    BLOG(0, "Missing signed creds");
    return mojom::Result::FAILED;
  }

  auto* public_key = dict.FindString("publicKey");
  if (!public_key) {
    BLOG(0, "Missing public key");
    return mojom::Result::FAILED;
  }

  base::JSONWriter::Write(*signed_creds, &batch->signed_creds);
  batch->public_key = *public_key;
  batch->batch_proof = *batch_proof;

  return mojom::Result::OK;
}

void GetSignedCreds::Request(const std::string& promotion_id,
                             const std::string& claim_id,
                             GetSignedCredsCallback callback) {
  auto url_callback = base::BindOnce(
      &GetSignedCreds::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(promotion_id, claim_id);
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void GetSignedCreds::OnRequest(GetSignedCredsCallback callback,
                               mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  mojom::CredsBatch batch;
  result = ParseBody(response->body, &batch);
  std::move(callback).Run(result, mojom::CredsBatch::New(batch));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
