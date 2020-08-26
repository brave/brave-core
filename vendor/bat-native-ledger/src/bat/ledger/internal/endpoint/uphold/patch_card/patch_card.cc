/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/patch_card/patch_card.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

PatchCard::PatchCard(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PatchCard::~PatchCard() = default;

std::string PatchCard::GetUrl(const std::string& address) {
  return GetServerUrl("/v0/me/cards/" + address);
}

std::string PatchCard::GeneratePayload(
    const braveledger_uphold::UpdateCard& card) {
  base::Value payload(base::Value::Type::DICTIONARY);

  if (!card.label.empty()) {
    payload.SetStringKey("label", card.label);
  }

  base::Value settings(base::Value::Type::DICTIONARY);
  if (card.position > -1) {
    settings.SetIntKey("position", card.position);
  }
  settings.SetBoolKey("starred", card.starred);
  payload.SetKey("settings", std::move(settings));

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

ledger::Result PatchCard::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

void PatchCard::Request(
    const std::string& token,
    const std::string& address,
    const braveledger_uphold::UpdateCard& card,
    PatchCardCallback callback) {
  auto url_callback = std::bind(&PatchCard::OnRequest,
      this,
      _1,
      callback);

  auto request = ledger::UrlRequest::New();
  request->url = GetUrl(address);
  request->content = GeneratePayload(card);
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = ledger::UrlMethod::PATCH;
  ledger_->LoadURL(std::move(request), url_callback);
}

void PatchCard::OnRequest(
    const ledger::UrlResponse& response,
    PatchCardCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace ledger
