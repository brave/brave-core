/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/request_signer.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {

using Error = GetWallet::Error;
using Result = GetWallet::Result;

namespace {

constexpr char kPath[] = "/v4/wallets/";

Result ParseBody(RewardsEngine& engine, const std::string& body) {
  auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  GetWallet::Value result_value;

  auto& dict = value->GetDict();

  if (auto* provider_dict = dict.FindDict("depositAccountProvider")) {
    const auto* name = provider_dict->FindString("name");
    const auto* id = provider_dict->FindString("id");
    const auto* linking_id = provider_dict->FindString("linkingId");

    if (!name || !id || !linking_id) {
      return base::unexpected(Error::kFailedToParseBody);
    }

    result_value.wallet_provider = *name;
    result_value.provider_id = *id;
    result_value.linked = !id->empty() && !linking_id->empty();
  }

  if (auto* self_custody_available = dict.FindDict("selfCustodyAvailable")) {
    for (auto&& [provider_name, dict_value] : *self_custody_available) {
      if (auto available = dict_value.GetIfBool()) {
        result_value.self_custody_available.Set(provider_name, *available);
      }
    }
  }

  return result_value;
}

}  // namespace

GetWalletValue::GetWalletValue() = default;
GetWalletValue::~GetWalletValue() = default;
GetWalletValue::GetWalletValue(GetWalletValue&&) = default;
GetWalletValue& GetWalletValue::operator=(GetWalletValue&&) = default;

// static
Result GetWallet::ProcessResponse(RewardsEngine& engine,
                                  const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(engine, response.body);
    case net::HTTP_BAD_REQUEST:  // HTTP 400
      engine.LogError(FROM_HERE) << "Invalid request";
      return base::unexpected(Error::kInvalidRequest);
    case net::HTTP_FORBIDDEN:  // HTTP 403
      engine.LogError(FROM_HERE) << "Request signature verification failure";
      return base::unexpected(Error::kRequestSignatureVerificationFailure);
    case net::HTTP_NOT_FOUND:  // HTTP 404
      engine.LogError(FROM_HERE) << "Rewards payment ID not found";
      return base::unexpected(Error::kRewardsPaymentIDNotFound);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetWallet::GetWallet(RewardsEngine& engine) : RequestBuilder(engine) {}

GetWallet::~GetWallet() = default;

std::optional<std::string> GetWallet::Url() const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
  }

  return engine_->Get<EnvironmentConfig>()
      .rewards_grant_url()
      .Resolve(base::StrCat({kPath, wallet->payment_id}))
      .spec();
}

mojom::UrlMethod GetWallet::Method() const {
  return mojom::UrlMethod::GET;
}

std::optional<std::vector<std::string>> GetWallet::Headers(
    const std::string& content) const {
  const auto wallet = engine_->wallet()->GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << "Rewards wallet is null";
    return std::nullopt;
  }

  DCHECK(!wallet->payment_id.empty());
  DCHECK(!wallet->recovery_seed.empty());

  auto signer = RequestSigner::FromRewardsWallet(*wallet);
  if (!signer) {
    engine_->LogError(FROM_HERE) << "Unable to sign request";
    return std::nullopt;
  }

  return signer->GetSignedHeaders(
      base::StrCat({"get ", kPath, wallet->payment_id}), content);
}

}  // namespace brave_rewards::internal::endpoints
