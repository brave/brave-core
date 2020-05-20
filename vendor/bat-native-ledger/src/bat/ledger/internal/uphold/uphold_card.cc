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
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto headers = RequestAuthorization(wallet->token);
  auto check_callback = std::bind(&UpholdCard::OnCreateIfNecessary,
                            this,
                            _1,
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
    const ledger::UrlResponse& response,
    const ledger::ExternalWallet& wallet,
    CreateCardCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_list()) {
    BLOG(0, "Response is not JSON");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  for (const auto& it : list->GetList()) {
    const auto* label = it.FindStringKey("label");
    if (!label) {
      continue;
    }

    if (*label == kCardName) {
      const auto* id = it.FindStringKey("id");
      if (!id) {
        continue;
      }

      callback(ledger::Result::LEDGER_OK, *id);
      return;
    }
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  Create(std::move(wallet_ptr), callback);
}

void UpholdCard::Create(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

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
    const ledger::UrlResponse& response,
    const ledger::ExternalWallet& wallet,
    CreateCardCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Response is not JSON");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Response is not JSON");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const auto* id = dictionary->FindStringKey("id");
  if (!id) {
    BLOG(0, "ID not found");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  wallet_ptr->address = *id;

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
    BLOG(0, "Card update failed");
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
    BLOG(0, "Wallet is null");
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
    const ledger::UrlResponse& response,
    UpdateCardCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN);
    uphold_->DisconnectWallet();
    return;
  }

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void UpholdCard::GetCardAddresses(
    ledger::ExternalWalletPtr wallet,
    GetCardAddressesCallback callback) {
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  const auto headers = RequestAuthorization(wallet->token);
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/addresses",
      wallet->address.c_str());

  auto address_callback = std::bind(&UpholdCard::OnGetCardAddresses,
      this,
      _1,
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
    BLOG(0, "JSON is not correct");
    return results;
  }

  base::ListValue* addresses = nullptr;
  if (!dictionary->GetAsList(&addresses)) {
    BLOG(0, "JSON is not correct");
    return results;
  }

  for (auto& address_item : *addresses) {
    base::DictionaryValue* address = nullptr;
    if (!address_item.GetAsDictionary(&address)) {
      continue;
    }

    const auto* type_key = address->FindStringKey("type");
    if (!type_key) {
      continue;
    }
    const std::string type = *type_key;

    auto* formats = address->FindListKey("formats");
    if (!formats) {
      continue;
    }

    if (formats->GetList().size() == 0) {
      continue;
    }

    base::DictionaryValue* format = nullptr;
    if (!formats->GetList()[0].GetAsDictionary(&format)) {
      continue;
    }

    const auto* address_value = format->FindStringKey("value");
    if (!address_value) {
      continue;
    }

    results.insert(std::make_pair(type, *address_value));
  }

  return results;
}

void UpholdCard::OnGetCardAddresses(
    const ledger::UrlResponse& response,
    GetCardAddressesCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN,  {});
    uphold_->DisconnectWallet();
    return;
  }

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR,  {});
    return;
  }

  auto result = ParseGetCardAddressResponse(response.body);
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
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

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
    const ledger::UrlResponse& response,
    CreateAnonAddressCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  if (response.status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN,  "");
    uphold_->DisconnectWallet();
    return;
  }

  if (response.status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR,  "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Response is not JSON");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Response is not JSON");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  const auto* id = dictionary->FindStringKey("id");
  if (!id) {
    BLOG(0, "ID not found");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  callback(ledger::Result::LEDGER_OK, *id);
}

}  // namespace braveledger_uphold
