/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/get_cards/get_cards.h"

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

GetCards::GetCards(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

GetCards::~GetCards() = default;

std::string GetCards::GetUrl() {
  return GetServerUrl("/v0/me/cards?q=currency:BAT");
}

ledger::Result GetCards::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result GetCards::ParseBody(
    const std::string& body,
    std::string* id) {
  DCHECK(id);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  for (const auto& it : list->GetList()) {
    const auto* label = it.FindStringKey("label");
    if (!label) {
      continue;
    }

    if (*label == braveledger_uphold::kCardName) {
      const auto* id_str = it.FindStringKey("id");
      if (!id_str) {
        continue;
      }

      *id = *id_str;
      return ledger::Result::LEDGER_OK;
    }
  }

  return ledger::Result::LEDGER_ERROR;
}

void GetCards::Request(
    const std::string& token,
    GetCardsCallback callback) {
  auto url_callback = std::bind(&GetCards::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      RequestAuthorization(token),
      "",
      "",
      ledger::UrlMethod::GET,
      url_callback);
}

void GetCards::OnRequest(
    const ledger::UrlResponse& response,
    GetCardsCallback callback) {
  ledger::LogUrlResponse(__func__, response);

  ledger::Result result = CheckStatusCode(response.status_code);

  if (result != ledger::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  std::string id;
  result = ParseBody(response.body, &id);
  callback(result, id);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
