/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

  UpdateCard::UpdateCard() :
    label(""),
    position(-1),
    starred(false) {}

  UpdateCard::~UpdateCard() {}

}  // namespace braveledger_uphold

namespace braveledger_uphold {

UpholdCard::UpholdCard(bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
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
                            *wallet,
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
    const ledger::ExternalWallet& wallet,
    CreateCardCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

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

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  wallet_ptr->address = id->GetString();

  auto update_callback = std::bind(&UpholdCard::OnCreateUpdate,
                                  this,
                                  _1,
                                  wallet_ptr->address,
                                  callback);
  UpdateCard card;
  card.starred = true;
  card.position = 1;
  Update(std::move(wallet_ptr), card, update_callback);
}

void UpholdCard::OnCreateUpdate(
    const ledger::Result result,
    const std::string& address,
    CreateCardCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  callback(result, address);
}

void UpholdCard::Update(
    ledger::ExternalWalletPtr wallet,
    const UpdateCard& card,
    UpdateCardCallback callback) {
  if (!wallet) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto headers = RequestAuthorization(wallet->token);

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

  const auto url = GetAPIUrl((std::string)"/v0/me/cards/" + wallet->address);
  auto update_callback = std::bind(&UpholdCard::OnUpdate,
                            this,
                            _1,
                            _2,
                            _3,
                            callback);

  ledger_->LoadURL(
    url,
    headers,
    json,
    "application/json",
    ledger::URL_METHOD::PATCH,
    update_callback);
}

void UpholdCard::OnUpdate(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    UpdateCardCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_uphold
