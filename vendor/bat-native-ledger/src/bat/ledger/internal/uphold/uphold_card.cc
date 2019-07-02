/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


namespace braveledger_uphold {

UpholdCard::UpholdCard(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

UpholdCard::~UpholdCard() {
}

void UpholdCard::Create(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  auto headers = RequestAuthorization(wallet->token);
  const std::string payload =
      "{ "
      "  \"label\": \"Brave Browser\", "
      "  \"currency\": \"BAT\" "
      "}";

  auto create_callback = std::bind(&UpholdCard::OnCreate,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);
  ledger_->LoadURL(
      GetAPIUrl("/v0/me/cards"),
      headers,
      payload,
      "application/json",
      ledger::URL_METHOD::POST,
      create_callback);
}

void UpholdCard::OnCreate(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    CreateCardCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto* id = dictionary->FindKey("id");
  std::string transaction_id;
  if (!id || !id->is_string()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, id->GetString());
}

}  // namespace braveledger_uphold
