/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/request_util.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = GetWallet::Error;
using Result = GetWallet::Result;

namespace {

Result ParseBody(const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto* deposit_account_provider =
      value->GetDict().FindDict("depositAccountProvider");
  if (!deposit_account_provider) {
    return std::pair{"" /* wallet provider */, false /* linked */};
  }

  auto* name = deposit_account_provider->FindString("name");
  const auto* id = deposit_account_provider->FindString("id");
  const auto* linking_id = deposit_account_provider->FindString("linkingId");

  if (!name || !id || !linking_id) {
    return base::unexpected(Error::kFailedToParseBody);
  }

  return std::pair{
      std::move(*name),                     // wallet provider
      !id->empty() && !linking_id->empty()  // linked
  };
}

}  // namespace

// static
Result GetWallet::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      BLOG(0, "Invalid request!");
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      BLOG(0, "Request signature verification failure!");
      return base::unexpected(Error::kRequestSignatureVerificationFailure);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      BLOG(0, "Rewards payment ID not found!");
      return base::unexpected(Error::kRewardsPaymentIDNotFound);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetWallet::GetWallet(RewardsEngineImpl& engine) : RequestBuilder(engine) {}

GetWallet::~GetWallet() = default;

std::string GetWallet::Path() const {
  return "/v4/wallets/";
}

absl::optional<std::string> GetWallet::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  return endpoint::promotion::GetServerUrl(Path() + wallet->payment_id);
}

mojom::UrlMethod GetWallet::Method() const {
  return mojom::UrlMethod::GET;
}

absl::optional<std::vector<std::string>> GetWallet::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Rewards wallet is null!");
    return absl::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  return util::BuildSignHeaders("get " + Path() + wallet->payment_id, content,
                                wallet->payment_id, wallet->recovery_seed);
}

}  // namespace brave_rewards::internal::endpoints
