/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_safetynet/post_safetynet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostSafetynet::PostSafetynet(RewardsEngineImpl& engine) : engine_(engine) {}

PostSafetynet::~PostSafetynet() = default;

std::string PostSafetynet::GetUrl() {
  return GetServerUrl("/v2/attestations/safetynet");
}

std::string PostSafetynet::GeneratePayload() {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value::List payment_ids;
  payment_ids.Append(base::Value(wallet->payment_id));

  base::Value::Dict body;
  body.Set("paymentIds", std::move(payment_ids));

  std::string json;
  base::JSONWriter::Write(body, &json);

  return json;
}

mojom::Result PostSafetynet::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Invalid token");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result PostSafetynet::ParseBody(const std::string& body,
                                       std::string* nonce) {
  DCHECK(nonce);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* nonce_string = dict.FindString("nonce");
  if (!nonce_string) {
    BLOG(0, "Nonce is wrong");
    return mojom::Result::FAILED;
  }

  *nonce = *nonce_string;
  return mojom::Result::OK;
}

void PostSafetynet::Request(PostSafetynetCallback callback) {
  auto url_callback = base::BindOnce(
      &PostSafetynet::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void PostSafetynet::OnRequest(PostSafetynetCallback callback,
                              mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  std::string nonce;
  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, nonce);
    return;
  }

  result = ParseBody(response->body, &nonce);
  std::move(callback).Run(result, nonce);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
