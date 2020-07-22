/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_api.h"

#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ledger/internal/logging.h"
#include "net/http/http_status_code.h"

namespace braveledger_response_util {

// Request Url:
// GET /v1/parameters
// GET /v1/parameters?currency={currency}
//
// Success:
// OK (200)
//
// Response Format:
// {
//   "batRate": 0.2476573499489187,
//   "autocontribute": {
//     "choices": [
//       5,
//       10,
//       15,
//       20,
//       25,
//       50,
//       100
//     ],
//     "defaultChoice": 20
//   },
//   "tips": {
//     "defaultTipChoices": [
//       1,
//       10,
//       100
//     ],
//     "defaultMonthlyChoices": [
//       1,
//       10,
//       100
//     ]
//   }
// }

ledger::Result ParseParameters(
    const ledger::UrlResponse& response,
    ledger::RewardsParameters* parameters) {
  DCHECK(parameters);

  // Bad Request (400)
  if (response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return ledger::Result::RETRY_SHORT;
  }

  // Internal Server Error (500)
  if (response.status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return ledger::Result::RETRY_SHORT;
  }

  if (response.status_code != net::HTTP_OK) {
    return ledger::Result::RETRY_SHORT;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto rate = dictionary->FindDoubleKey("batRate");
  if (!rate) {
    BLOG(0, "Missing BAT rate");
    return ledger::Result::LEDGER_ERROR;
  }
  parameters->rate = *rate;

  const auto ac_choice =
      dictionary->FindDoublePath("autocontribute.defaultChoice");
  if (!ac_choice) {
    BLOG(0, "Invalid auto-contribute default choice");
    return ledger::Result::LEDGER_ERROR;
  }
  parameters->auto_contribute_choice = *ac_choice;

  auto* ac_choices = dictionary->FindListPath("autocontribute.choices");
  if (!ac_choices || ac_choices->GetList().empty()) {
    BLOG(0, "Missing auto-contribute choices");
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : ac_choices->GetList()) {
    parameters->auto_contribute_choices.push_back(choice.GetDouble());
  }

  auto* tip_choices = dictionary->FindListPath("tips.defaultTipChoices");
  if (!tip_choices || tip_choices->GetList().empty()) {
    BLOG(0, "Missing default tip choices");
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : tip_choices->GetList()) {
    parameters->tip_choices.push_back(choice.GetDouble());
  }

  auto* monthly_tip_choices =
      dictionary->FindListPath("tips.defaultMonthlyChoices");
  if (!monthly_tip_choices || monthly_tip_choices->GetList().empty()) {
    BLOG(0, "Missing tips default monthly choices");
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& choice : monthly_tip_choices->GetList()) {
    parameters->monthly_tip_choices.push_back(choice.GetDouble());
  }

  return ledger::Result::LEDGER_OK;
}

}  // namespace braveledger_response_util
