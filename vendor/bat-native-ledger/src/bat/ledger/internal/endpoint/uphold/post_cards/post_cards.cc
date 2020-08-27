/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/uphold/post_cards/post_cards.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_utils.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace uphold {

PostCards::PostCards(bat_ledger::LedgerImpl* ledger):
    ledger_(ledger) {
  DCHECK(ledger_);
}

PostCards::~PostCards() = default;

std::string PostCards::GetUrl() {
  return GetServerUrl("/v0/me/cards");
}

std::string PostCards::GeneratePayload() {
  base::Value payload(base::Value::Type::DICTIONARY);
  payload.SetStringKey("label", braveledger_uphold::kCardName);
  payload.SetStringKey("currency", "BAT");

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

ledger::Result PostCards::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    return ledger::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    return ledger::Result::LEDGER_ERROR;
  }

  return ledger::Result::LEDGER_OK;
}

ledger::Result PostCards::ParseBody(
    const std::string& body,
    std::string* id) {
  DCHECK(id);

  base::Optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    return ledger::Result::LEDGER_ERROR;
  }

  const auto* id_str = dictionary->FindStringKey("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return ledger::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return ledger::Result::LEDGER_OK;
}

void PostCards::Request(
    const std::string& token,
    PostCardsCallback callback) {
  auto url_callback = std::bind(&PostCards::OnRequest,
      this,
      _1,
      callback);
  ledger_->LoadURL(
      GetUrl(),
      RequestAuthorization(token),
      GeneratePayload(),
      "application/json; charset=utf-8",
      ledger::UrlMethod::POST,
      url_callback);
}

void PostCards::OnRequest(
    const ledger::UrlResponse& response,
    PostCardsCallback callback) {
  ledger::LogUrlResponse(__func__, response, true);

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
