/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoints/get_parameters/get_parameters.h"

#include <utility>

#include "base/json/json_reader.h"
#include "bat/ledger/internal/endpoint/api/api_util.h"
#include "bat/ledger/internal/endpoints/get_parameters/get_parameters_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger::endpoints {
using Error = GetParameters::Error;
using Result = GetParameters::Result;

namespace {

Result ParseBody(const std::string& body) {
  const auto value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  const auto& dict = value->GetDict();

  const auto rate = dict.FindDouble("batRate");
  if (!rate) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }
  auto parameters = mojom::RewardsParameters::New();
  parameters->rate = *rate;

  const auto auto_contribute_choice =
      dict.FindDoubleByDottedPath("autocontribute.defaultChoice");
  if (!auto_contribute_choice) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }
  parameters->auto_contribute_choice = *auto_contribute_choice;

  const auto* auto_contribute_choices =
      dict.FindListByDottedPath("autocontribute.choices");
  if (!auto_contribute_choices || auto_contribute_choices->empty()) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto& choice : *auto_contribute_choices) {
    if (choice.is_double() || choice.is_int()) {
      parameters->auto_contribute_choices.push_back(choice.GetDouble());
    }
  }

  const auto* tip_choices = dict.FindListByDottedPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->empty()) {
    BLOG(0, "Failed to parse body!");
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
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto& choice : *monthly_tip_choices) {
    if (choice.is_double() || choice.is_int()) {
      parameters->monthly_tip_choices.push_back(choice.GetDouble());
    }
  }

  const auto* payout_status = dict.FindDict("payoutStatus");
  if (!payout_status) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  for (const auto [k, v] : *payout_status) {
    if (v.is_string()) {
      parameters->payout_status.emplace(k, v.GetString());
    }
  }

  const auto* custodian_regions = dict.FindDict("custodianRegions");
  if (!custodian_regions) {
    BLOG(0, "Failed to parse body!");
    return base::unexpected(Error::kFailedToParseBody);
  }

  auto wallet_provider_regions = GetWalletProviderRegions(*custodian_regions);
  if (!wallet_provider_regions) {
    BLOG(0, "Failed to parse body!");
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
Result GetParameters::ProcessResponse(const mojom::UrlResponse& response) {
  switch (response.status_code) {
    case net::HTTP_OK:  // HTTP 200
      return ParseBody(response.body);
    case net::HTTP_INTERNAL_SERVER_ERROR:  // HTTP 500
      BLOG(0, "Failed to get parameters!");
      return base::unexpected(Error::kFailedToGetParameters);
    default:
      BLOG(0, "Unexpected status code! (HTTP " << response.status_code << ')');
      return base::unexpected(Error::kUnexpectedStatusCode);
  }
}

GetParameters::GetParameters(LedgerImpl* ledger) : RequestBuilder(ledger) {}

GetParameters::~GetParameters() = default;

absl::optional<std::string> GetParameters::Url() const {
  return endpoint::api::GetServerUrl("/v1/parameters");
}

mojom::UrlMethod GetParameters::Method() const {
  return mojom::UrlMethod::GET;
}

}  // namespace ledger::endpoints
