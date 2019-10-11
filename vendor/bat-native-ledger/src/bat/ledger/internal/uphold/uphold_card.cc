/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
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

void UpholdCard::CreateIfNecessary(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  auto headers = RequestAuthorization(wallet->token);
  auto check_callback = std::bind(&UpholdCard::OnCreateIfNecessary,
                            this,
                            _1,
                            _2,
                            _3,
                            *wallet,
                            callback);
  ledger_->LoadURL(
      GetAPIUrl("/v0/me/cards?q=currency:BAT"),
      headers,
      "",
      "application/json",
      ledger::UrlMethod::GET,
      check_callback);
}

void UpholdCard::OnCreateIfNecessary(
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
  if (!value || !value->is_list()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  for (const auto& it : list->GetList()) {
    auto* label = it.FindKey("label");
    if (!label || !label->is_string()) {
      continue;
    }

    if (label->GetString() == kCardName) {
      auto* id = it.FindKey("id");
      if (!id || !id->is_string()) {
        continue;
      }

      callback(ledger::Result::LEDGER_OK, id->GetString());
      return;
    }
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  Create(std::move(wallet_ptr), callback);
}

void UpholdCard::Create(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  auto headers = RequestAuthorization(wallet->token);
  const std::string payload =
      base::StringPrintf(
          "{ "
          "  \"label\": \"%s\", "
          "  \"currency\": \"BAT\" "
          "}",
          kCardName);

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
      ledger::UrlMethod::POST,
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
    ledger::UrlMethod::PATCH,
    update_callback);
}

void UpholdCard::OnUpdate(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    UpdateCardCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN);
    uphold_->DisconnectWallet();
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void UpholdCard::GetCardAddresses(
    ledger::ExternalWalletPtr wallet,
    GetCardAddressesCallback callback) {
  const auto headers = RequestAuthorization(wallet->token);
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/addresses",
      wallet->address.c_str());

  auto address_callback = std::bind(&UpholdCard::OnGetCardAddresses,
      this,
      _1,
      _2,
      _3,
      callback);

  const auto url = GetAPIUrl(path);
  ledger_->LoadURL(
      url,
      headers,
      "",
      "application/json",
      ledger::UrlMethod::GET,
      address_callback);
}

std::map<std::string, std::string> UpholdCard::ParseGetCardAddressResponse(
    const std::string& response) {
  std::map<std::string, std::string> results;

  base::Optional<base::Value> dictionary = base::JSONReader::Read(response);
  if (!dictionary || !dictionary->is_list()) {
    return results;
  }

  base::ListValue* addresses = nullptr;
  if (!dictionary->GetAsList(&addresses)) {
    return results;
  }

  for (auto& address_item : *addresses) {
    base::DictionaryValue* address = nullptr;
    if (!address_item.GetAsDictionary(&address)) {
      continue;
    }

    auto* type_key = address->FindKey("type");
    if (!type_key || !type_key->is_string()) {
      continue;
    }
    const std::string type = type_key->GetString();

    auto* formats_key = address->FindKey("formats");
    if (!formats_key || !formats_key->is_list()) {
      continue;
    }

    base::ListValue* formats = nullptr;
    if (!formats_key->GetAsList(&formats)) {
      continue;
    }

    if (formats->GetList().size() == 0) {
      continue;
    }

    base::DictionaryValue* format = nullptr;
    if (!formats->GetList()[0].GetAsDictionary(&format)) {
      continue;
    }

    auto* value = format->FindKey("value");
    if (!value || !value->is_string()) {
      continue;
    }
    const std::string address_value = value->GetString();

    results.insert(std::make_pair(type, address_value));
  }

  return results;
}

void UpholdCard::OnGetCardAddresses(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    GetCardAddressesCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN,  {});
    uphold_->DisconnectWallet();
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR,  {});
    return;
  }

  auto result = ParseGetCardAddressResponse(response);
  callback(ledger::Result::LEDGER_OK, result);
}

void UpholdCard::CreateAnonAddressIfNecessary(
    ledger::ExternalWalletPtr wallet,
    CreateAnonAddressCallback callback) {
  auto address_callback = std::bind(&UpholdCard::OnCreateAnonAddressIfNecessary,
      this,
      _1,
      _2,
      *wallet,
      callback);

  GetCardAddresses(std::move(wallet), address_callback);
}

void UpholdCard::OnCreateAnonAddressIfNecessary(
    ledger::Result result,
    std::map<std::string, std::string> addresses,
    const ledger::ExternalWallet& wallet,
    CreateAnonAddressCallback callback) {
  if (result == ledger::Result::LEDGER_OK && addresses.size() > 0) {
    auto iter = addresses.find(kAnonID);
    if (iter != addresses.end() && !iter->second.empty()) {
      callback(ledger::Result::LEDGER_OK, iter->second);
      return;
    }
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  CreateAnonAddress(std::move(wallet_ptr), callback);
}

void UpholdCard::CreateAnonAddress(
    ledger::ExternalWalletPtr wallet,
    CreateAnonAddressCallback callback) {
  const auto headers = RequestAuthorization(wallet->token);
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/addresses",
      wallet->address.c_str());

  const std::string payload = base::StringPrintf(
      "{ "
      "  \"network\": \"%s\" "
      "}",
      kAnonID);

  auto anon_callback = std::bind(&UpholdCard::OnCreateAnonAddress,
      this,
      _1,
      _2,
      _3,
      callback);

  const auto url = GetAPIUrl(path);
  ledger_->LoadURL(
      url,
      headers,
      payload,
      "application/json",
      ledger::UrlMethod::POST,
      anon_callback);
}

void UpholdCard::OnCreateAnonAddress(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers,
    CreateAnonAddressCallback callback) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN,  "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR,  "");
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
  if (!id || !id->is_string()) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, id->GetString());
}

}  // namespace braveledger_uphold
