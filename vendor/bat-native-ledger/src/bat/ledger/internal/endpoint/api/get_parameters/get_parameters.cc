/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/api/get_parameters/get_parameters.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/api/api_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace ledger {
namespace endpoint {
namespace api {

GetParameters::GetParameters(LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetParameters::~GetParameters() = default;

std::string GetParameters::GetUrl(const std::string& currency) {
  std::string query;
  if (!currency.empty()) {
    query = base::StringPrintf("?currency=%s", currency.c_str());
  }

  const std::string path = base::StringPrintf(
      "/v1/parameters%s",
      query.c_str());

  return GetServerUrl(path);
}

mojom::Result GetParameters::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::RETRY_SHORT;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result GetParameters::ParseBody(const std::string& body,
                                       mojom::RewardsParameters* parameters) {
  DCHECK(parameters);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto rate = dict.FindDouble("batRate");
  if (!rate) {
    BLOG(0, "Missing BAT rate");
    return mojom::Result::LEDGER_ERROR;
  }
  parameters->rate = *rate;

  const auto ac_choice =
      dict.FindDoubleByDottedPath("autocontribute.defaultChoice");
  if (!ac_choice) {
    BLOG(0, "Invalid auto-contribute default choice");
    return mojom::Result::LEDGER_ERROR;
  }
  parameters->auto_contribute_choice = *ac_choice;

  auto* ac_choices = dict.FindListByDottedPath("autocontribute.choices");
  if (!ac_choices || ac_choices->empty()) {
    BLOG(0, "Missing auto-contribute choices");
    return mojom::Result::LEDGER_ERROR;
  }

  for (const auto& choice : *ac_choices) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->auto_contribute_choices.push_back(choice.GetDouble());
  }

  auto* tip_choices = dict.FindListByDottedPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->empty()) {
    BLOG(0, "Missing default tip choices");
    return mojom::Result::LEDGER_ERROR;
  }

  for (const auto& choice : *tip_choices) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->tip_choices.push_back(choice.GetDouble());
  }

  auto* monthly_tip_choices =
      dict.FindListByDottedPath("tips.defaultMonthlyChoices");
  if (!monthly_tip_choices || monthly_tip_choices->empty()) {
    BLOG(0, "Missing tips default monthly choices");
    return mojom::Result::LEDGER_ERROR;
  }

  for (const auto& choice : *monthly_tip_choices) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->monthly_tip_choices.push_back(choice.GetDouble());
  }

  const auto* payout_status_dict = dict.FindDict("payoutStatus");
  if (!payout_status_dict) {
    BLOG(0, "Missing payout status");
    return mojom::Result::LEDGER_ERROR;
  }

  for (auto&& [k, v] : *payout_status_dict) {
    if (v.is_string()) {
      parameters->payout_status.emplace(k, v.GetString());
    }
  }

  return mojom::Result::LEDGER_OK;
}

void GetParameters::Request(GetParametersCallback callback) {
  auto url_callback = base::BindOnce(
      &GetParameters::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  ledger_->LoadURL(std::move(request), std::move(url_callback));
}

void GetParameters::OnRequest(GetParametersCallback callback,
                              const mojom::UrlResponse& response) {
  ledger::LogUrlResponse(__func__, response);

  mojom::RewardsParameters parameters;
  mojom::Result result = CheckStatusCode(response.status_code);

  if (result != mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, parameters);
    return;
  }

  result = ParseBody(response.body, &parameters);
  std::move(callback).Run(result, parameters);
}

}  // namespace api
}  // namespace endpoint
}  // namespace ledger
