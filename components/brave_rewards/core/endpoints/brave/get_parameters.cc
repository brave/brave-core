/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters_utils.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {
using Error = GetParameters::Error;
using Result = GetParameters::Result;

namespace {

Result ParseBody(RewardsEngineImpl& engine, const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto& dict = value->GetDict();

  const auto rate = dict.FindDouble("batRate");
  if (!rate) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }
  auto parameters = mojom::RewardsParameters::New();
  parameters->rate = *rate;

  const auto auto_contribute_choice =
      dict.FindDoubleByDottedPath("autocontribute.defaultChoice");
  if (!auto_contribute_choice) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }
  parameters->auto_contribute_choice = *auto_contribute_choice;

  const auto* auto_contribute_choices =
      dict.FindListByDottedPath("autocontribute.choices");
  if (!auto_contribute_choices || auto_contribute_choices->empty()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto& choice : *auto_contribute_choices) {
    if (choice.is_double() || choice.is_int()) {
      parameters->auto_contribute_choices.push_back(choice.GetDouble());
    }
  }

  const auto* tip_choices = dict.FindListByDottedPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->empty()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto& choice : *tip_choices) {
    if (choice.is_double() || choice.is_int()) {
      parameters->tip_choices.push_back(choice.GetDouble());
    }
  }

  const auto* monthly_tip_choices =
      dict.FindListByDottedPath("tips.defaultMonthlyChoices");
  if (!monthly_tip_choices || monthly_tip_choices->empty()) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto& choice : *monthly_tip_choices) {
    if (choice.is_double() || choice.is_int()) {
      parameters->monthly_tip_choices.push_back(choice.GetDouble());
    }
  }

  const auto* payout_status = dict.FindDict("payoutStatus");
  if (!payout_status) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto [k, v] : *payout_status) {
    if (v.is_string()) {
      parameters->payout_status.emplace(k, v.GetString());
    }
  }

  const auto* custodian_regions = dict.FindDict("custodianRegions");
  if (!custodian_regions) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto wallet_provider_regions = GetWalletProviderRegions(*custodian_regions);
  if (!wallet_provider_regions) {
    engine.LogError(FROM_HERE) << "Failed to parse body";
    return base::unexpected(Error::kFailedToParseBody);
  }

  parameters->wallet_provider_regions = std::move(*wallet_provider_regions);

  const auto* vbat_deadline = dict.FindString("vbatDeadline");
  if (vbat_deadline) {
    if (base::Time time;
        base::Time::FromUTCString(vbat_deadline->c_str(), &time)) {
      parameters->vbat_deadline = time;
    }
  }

  const auto vbat_expired = dict.FindBool("vbatExpired");
  if (vbat_expired) {
    parameters->vbat_expired = *vbat_expired;
  }

  return parameters;
}

}  // namespace

// static
Result GetParameters::ProcessResponse(RewardsEngineImpl& engine,
                                      const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(engine, response.body);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      engine.LogError(FROM_HERE) << "Failed to get parameters";
      return base::unexpected(Error::kFailedToGetParameters);
    default:
      engine.LogError(FROM_HERE)
          << "Unexpected status code! (HTTP " << response.status_code << ')';
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetParameters::GetParameters(RewardsEngineImpl& engine)
    : RequestBuilder(engine) {}

GetParameters::~GetParameters() = default;

std::optional<std::string> GetParameters::Url() const {
  return engine_->Get<EnvironmentConfig>()
      .rewards_api_url()
      .Resolve("/v1/parameters")
      .spec();
}

mojom::UrlMethod GetParameters::Method() const {
  return mojom::UrlMethod::GET;
}

}  // namespace brave_rewards::internal::endpoints
