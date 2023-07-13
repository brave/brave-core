/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/request_util.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_creds/post_creds.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostCreds::PostCreds(RewardsEngineImpl& engine) : engine_(engine) {}

PostCreds::~PostCreds() = default;

std::string PostCreds::GetUrl(const std::string& promotion_id) {
  const std::string& path =
      base::StringPrintf("/v1/promotions/%s", promotion_id.c_str());

  return GetServerUrl(path);
}

std::string PostCreds::GeneratePayload(base::Value::List&& blinded_creds) {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value::Dict body;
  body.Set("paymentId", wallet->payment_id);
  body.Set("blindedCreds", base::Value(std::move(blinded_creds)));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostCreds::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Signature validation failed");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_CONFLICT) {
    BLOG(0, "Incorrect blinded credentials");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_GONE) {
    BLOG(0, "Promotion is gone");
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

mojom::Result PostCreds::ParseBody(const std::string& body,
                                   std::string* claim_id) {
  DCHECK(claim_id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* id = dict.FindString("claimId");
  if (!id || id->empty()) {
    BLOG(0, "Claim id is missing");
    return mojom::Result::FAILED;
  }

  *claim_id = *id;

  return mojom::Result::OK;
}

void PostCreds::Request(const std::string& promotion_id,
                        base::Value::List&& blinded_creds,
                        PostCredsCallback callback) {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  const std::string& payload = GeneratePayload(std::move(blinded_creds));

  const auto headers =
      util::BuildSignHeaders("post /v1/promotions/" + promotion_id, payload,
                             wallet->payment_id, wallet->recovery_seed);

  auto url_callback = base::BindOnce(
      &PostCreds::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(promotion_id);
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void PostCreds::OnRequest(PostCredsCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  std::string claim_id;
  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, claim_id);
    return;
  }

  result = ParseBody(response->body, &claim_id);
  std::move(callback).Run(result, claim_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
