/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/request_util.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/credentials/credentials_util.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

PostSuggestionsClaim::PostSuggestionsClaim(RewardsEngineImpl& engine)
    : engine_(engine) {}

PostSuggestionsClaim::~PostSuggestionsClaim() = default;

std::string PostSuggestionsClaim::GetUrl() {
  return GetServerUrl("/v2/suggestions/claim");
}

std::string PostSuggestionsClaim::GeneratePayload(
    const credential::CredentialsRedeem& redeem) {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value::List credentials =
      credential::GenerateCredentials(redeem.token_list, wallet->payment_id);

  base::Value::Dict body;
  body.Set("paymentId", wallet->payment_id);
  body.Set("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

mojom::Result PostSuggestionsClaim::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return mojom::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

void PostSuggestionsClaim::Request(const credential::CredentialsRedeem& redeem,
                                   PostSuggestionsClaimCallback callback) {
  const std::string payload = GeneratePayload(redeem);

  auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  auto url_callback =
      base::BindOnce(&PostSuggestionsClaim::OnRequest, base::Unretained(this),
                     std::move(callback));

  auto headers =
      util::BuildSignHeaders("post /v2/suggestions/claim", payload,
                             wallet->payment_id, wallet->recovery_seed);

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void PostSuggestionsClaim::OnRequest(PostSuggestionsClaimCallback callback,
                                     mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);
  auto result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, "");
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(response->body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }

  const base::Value::Dict& dict = value->GetDict();
  auto* drain_id = dict.FindString("drainId");
  if (!drain_id) {
    BLOG(0, "Missing drain id");
    std::move(callback).Run(mojom::Result::FAILED, "");
    return;
  }
  std::move(callback).Run(result, std::move(*drain_id));
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
