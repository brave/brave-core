/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "bat/ledger/internal/api/api_parameters.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/request/request_api.h"
#include "bat/ledger/internal/state/state_util.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace {

ledger::Result ParseFetchResponse(
    const std::string& response,
    ledger::RewardsParameters* parameters) {
  DCHECK(parameters);

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return ledger::Result::LEDGER_ERROR;
  }

  const auto rate = dictionary->FindDoubleKey("batRate");
  if (!rate) {
    return ledger::Result::LEDGER_ERROR;
  }
  parameters->rate = *rate;

  const auto ac_choice =
      dictionary->FindDoublePath("autocontribute.defaultChoice");
  if (!ac_choice) {
    return ledger::Result::LEDGER_ERROR;
  }
  parameters->auto_contribute_choice = *ac_choice;

  auto* ac_choices = dictionary->FindListPath("autocontribute.choices");
  if (!ac_choices || ac_choices->GetList().empty()) {
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : ac_choices->GetList()) {
    parameters->auto_contribute_choices.push_back(choice.GetDouble());
  }

  auto* tip_choices = dictionary->FindListPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->GetList().empty()) {
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : tip_choices->GetList()) {
    parameters->tip_choices.push_back(choice.GetDouble());
  }

  auto* monthly_tip_choices =
      dictionary->FindListPath("tips.defaultMonthlyChoices");
  if (!monthly_tip_choices || monthly_tip_choices->GetList().empty()) {
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : monthly_tip_choices->GetList()) {
    parameters->monthly_tip_choices.push_back(choice.GetDouble());
  }

  return ledger::Result::LEDGER_OK;
}

}  // namespace

namespace braveledger_api {

APIParameters::APIParameters(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
  DCHECK(ledger_);
}

APIParameters::~APIParameters() = default;

void APIParameters::Initialize() {
  if (braveledger_state::GetRewardsMainEnabled(ledger_)) {
    Fetch();
  }
}

void APIParameters::OnTimer(const uint32_t timer_id) {
  if (timer_id == refresh_timer_id_) {
    refresh_timer_id_ = 0;
    Fetch();
    return;
  }
}

void APIParameters::Fetch() {
  auto url_callback = std::bind(&APIParameters::OnFetch,
      this,
      _1);

  const std::string url = braveledger_request_util::GetParametersURL();
  ledger_->LoadURL(url, {}, "", "", ledger::UrlMethod::GET, url_callback);
}

void APIParameters::OnFetch(const ledger::UrlResponse& response) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code != net::HTTP_OK) {
    SetRefreshTimer(brave_base::random::Geometric(90));
    return;
  }

  ledger::RewardsParameters parameters;
  auto result = ParseFetchResponse(response.body, &parameters);
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(1, "Couldn't parse response");
    SetRefreshTimer(
        brave_base::random::Geometric(10 * base::Time::kSecondsPerMinute));
    return;
  }

  braveledger_state::SetRewardsParameters(ledger_, parameters);
  SetRefreshTimer();
}

void APIParameters::SetRefreshTimer(const int delay) {
  if (refresh_timer_id_ != 0) {
    BLOG(1, "Params timer in progress");
    return;
  }

  int64_t start_in = delay;
  if (delay == 0) {
    const int64_t base_delay = 3 * base::Time::kSecondsPerHour;
    const int64_t random_delay =
        brave_base::random::Geometric(10 * base::Time::kSecondsPerMinute);
    start_in = base_delay + random_delay;
  }

  BLOG(1, "Params timer start in " << start_in);

  ledger_->SetTimer(start_in, &refresh_timer_id_);
}


}  // namespace braveledger_api
