/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_challenges.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {

PostChallenges::PostChallenges(RewardsEngineImpl& engine)
    : RewardsEngineHelper(engine) {}

PostChallenges::~PostChallenges() = default;

void PostChallenges::Request(RequestCallback callback) {
  auto request = CreateRequest();
  if (!request) {
    DeferCallback(FROM_HERE, std::move(callback),
                  base::unexpected(Error::kInvalidRequest));
    return;
  }

  Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kBasic,
      base::BindOnce(&PostChallenges::OnResponse, weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

mojom::UrlRequestPtr PostChallenges::CreateRequest() {
  auto request = mojom::UrlRequest::New();

  request->method = mojom::UrlMethod::POST;
  request->url = Get<EnvironmentConfig>()
                     .rewards_grant_url()
                     .Resolve("/v3/wallet/challenges")
                     .spec();
  request->content_type = "application/json";

  auto rewards_wallet = engine().wallet()->GetWallet();
  if (!rewards_wallet) {
    LogError(FROM_HERE) << "Rewards wallet is null";
    return nullptr;
  }

  base::Value::Dict content_data;
  content_data.Set("paymentId", rewards_wallet->payment_id);

  if (!base::JSONWriter::Write(content_data, &request->content)) {
    LogError(FROM_HERE) << "Failed to write content to JSON";
    return nullptr;
  }

  auto request_signer = RequestSigner::FromRewardsWallet(*rewards_wallet);
  if (!request_signer || !request_signer->SignRequest(*request)) {
    LogError(FROM_HERE) << "Unable to sign request";
    return nullptr;
  }

  return request;
}

PostChallenges::Result PostChallenges::MapResponse(
    const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_CREATED:  // HTTP 201
      break;
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      LogError(FROM_HERE) << "Bad request";
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      LogError(FROM_HERE) << "Server error";
      return base::unexpected(Error::kUnexpectedError);
    default:
      LogError(FROM_HERE) << "Unexpected status code: " << response.status_code;
      return base::unexpected(Error::kUnexpectedStatusCode);
  }

  auto value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    LogError(FROM_HERE) << "Failed to parse body: invalid JSON";
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* challenge_id = value->GetDict().FindString("challengeId");
  if (!challenge_id || challenge_id->empty()) {
    LogError(FROM_HERE) << "Failed to parse body: missing challengeId";
    return base::unexpected(Error::kFailedToParseBody);
  }

  return *challenge_id;
}

void PostChallenges::OnResponse(RequestCallback callback,
                                mojom::UrlResponsePtr response) {
  std::move(callback).Run(MapResponse(*response));
}

}  // namespace brave_rewards::internal::endpoints
