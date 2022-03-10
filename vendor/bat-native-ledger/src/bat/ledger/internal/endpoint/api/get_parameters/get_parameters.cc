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

using std::placeholders::_1;

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

type::Result GetParameters::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::RETRY_SHORT;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return type::Result::RETRY_SHORT;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

type::Result GetParameters::ParseBody(
    const std::string& body,
    type::RewardsParameters* parameters) {
  DCHECK(parameters);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return type::Result::LEDGER_ERROR;
  }

  const auto rate = dictionary->FindDoubleKey("batRate");
  if (!rate) {
    BLOG(0, "Missing BAT rate");
    return type::Result::LEDGER_ERROR;
  }
  parameters->rate = *rate;

  const auto ac_choice =
      dictionary->FindDoublePath("autocontribute.defaultChoice");
  if (!ac_choice) {
    BLOG(0, "Invalid auto-contribute default choice");
    return type::Result::LEDGER_ERROR;
  }
  parameters->auto_contribute_choice = *ac_choice;

  auto* ac_choices = dictionary->FindListPath("autocontribute.choices");
  if (!ac_choices || ac_choices->GetList().empty()) {
    BLOG(0, "Missing auto-contribute choices");
    return type::Result::LEDGER_ERROR;
  }

  for (const auto& choice : ac_choices->GetList()) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->auto_contribute_choices.push_back(choice.GetDouble());
  }

  auto* tip_choices = dictionary->FindListPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->GetList().empty()) {
    BLOG(0, "Missing default tip choices");
    return type::Result::LEDGER_ERROR;
  }

  for (const auto& choice : tip_choices->GetList()) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->tip_choices.push_back(choice.GetDouble());
  }

  auto* monthly_tip_choices =
      dictionary->FindListPath("tips.defaultMonthlyChoices");
  if (!monthly_tip_choices || monthly_tip_choices->GetList().empty()) {
    BLOG(0, "Missing tips default monthly choices");
    return type::Result::LEDGER_ERROR;
  }

  for (const auto& choice : monthly_tip_choices->GetList()) {
    if (!choice.is_double() && !choice.is_int()) {
      continue;
    }
    parameters->monthly_tip_choices.push_back(choice.GetDouble());
  }

  return type::Result::LEDGER_OK;
}

void GetParameters::Request(GetParametersCallback callback) {
  auto url_callback = std::bind(&GetParameters::OnRequest,
      this,
      _1,
      callback);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  ledger_->LoadURL(std::move(request), url_callback);
}

void GetParameters::OnRequest(
    const type::UrlResponse& response,
    GetParametersCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  type::RewardsParameters parameters;
  type::Result result = CheckStatusCode(response.status_code);

  if (result != type::Result::LEDGER_OK) {
    callback(result, parameters);
    return;
  }

  result = ParseBody(response.body, &parameters);
  callback(result, parameters);
}

}  // namespace api
}  // namespace endpoint
}  // namespace ledger
