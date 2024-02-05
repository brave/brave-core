/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/get_signed_creds/get_signed_creds.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

GetSignedCreds::GetSignedCreds(RewardsEngineImpl& engine) : engine_(engine) {}

GetSignedCreds::~GetSignedCreds() = default;

std::string GetSignedCreds::GetUrl(const std::string& promotion_id,
                                   const std::string& claim_id) {
  auto url = URLHelpers::Resolve(
      engine_->Get<EnvironmentConfig>().rewards_grant_url(),
      {"/v1/promotions/", promotion_id, "/claims/", claim_id});
  return url.spec();
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

  std::optional<base::Value> value = base::JSONReader::Read(body);
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

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void GetSignedCreds::OnRequest(GetSignedCredsCallback callback,
                               mojom::UrlResponsePtr response) {
  DCHECK(response);

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
