/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/response/response_uphold.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

UpdateCard::UpdateCard() :
  label(""),
  position(-1),
  starred(false) {}

UpdateCard::~UpdateCard() = default;

}  // namespace braveledger_uphold

namespace braveledger_uphold {

UpholdCard::UpholdCard(bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateIfNecessary(CreateCardCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto headers = RequestAuthorization(wallet->token);
  auto check_callback = std::bind(&UpholdCard::OnCreateIfNecessary,
      this,
      _1,
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
    CreateCardCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string id;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdGetCards(response, kCardName, &id);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (result == ledger::Result::LEDGER_OK && !id.empty()) {
    callback(ledger::Result::LEDGER_OK, id);
    return;
  }

  Create(callback);
}

void UpholdCard::Create(
    CreateCardCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto headers = RequestAuthorization(wallet->token);
  const std::string payload = base::StringPrintf(
      R"({
        "label": "%s",
        "currency": "BAT"
      })",
      kCardName);

  auto create_callback = std::bind(&UpholdCard::OnCreate,
      this,
      _1,
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
    CreateCardCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string id;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdGetCard(response, &id);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(ledger::Result::EXPIRED_TOKEN, "");
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK || id.empty()) {
    BLOG(0, "Couldn't create anon card address");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = GetWallet(std::move(wallets));
  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, "");
    return;
  }
  wallet_ptr->address = id;
  ledger_->ledger_client()->SaveExternalWallet(
      ledger::kWalletUphold,
      wallet_ptr->Clone());

  auto update_callback = std::bind(&UpholdCard::OnCreateUpdate,
      this,
      _1,
      wallet_ptr->address,
      callback);
  UpdateCard card;
  card.starred = true;
  card.position = 1;
  Update(card, update_callback);
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
    const UpdateCard& card,
    UpdateCardCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
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

  std::string id;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdCreateCard(response, &id);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(ledger::Result::EXPIRED_TOKEN);
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Couldn't update rewards card");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

void UpholdCard::GetCardAddresses(
    GetCardAddressesCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
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

void UpholdCard::OnGetCardAddresses(
    const ledger::UrlResponse& response,
    GetCardAddressesCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::map<std::string, std::string> addresses;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdGetCardAddresses(
          response,
          &addresses);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(ledger::Result::EXPIRED_TOKEN, {});
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get card addresses");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  callback(ledger::Result::LEDGER_OK, addresses);
}

void UpholdCard::CreateAnonAddressIfNecessary(ledger::ResultCallback callback) {
  auto address_callback = std::bind(&UpholdCard::OnCreateAnonAddressIfNecessary,
      this,
      _1,
      _2,
      callback);

  GetCardAddresses(address_callback);
}

void UpholdCard::OnCreateAnonAddressIfNecessary(
    ledger::Result result,
    std::map<std::string, std::string> addresses,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::LEDGER_OK && addresses.size() > 0) {
    auto iter = addresses.find(kAnonID);
    if (iter != addresses.end() && !iter->second.empty()) {
      auto wallets = ledger_->ledger_client()->GetExternalWallets();
      auto wallet = GetWallet(std::move(wallets));
      if (!wallet) {
        BLOG(0, "Wallet is null");
        callback(ledger::Result::LEDGER_ERROR);
        return;
      }

      wallet->anon_address = iter->second;
      ledger_->ledger_client()->SaveExternalWallet(
          ledger::kWalletUphold,
          std::move(wallet));
      callback(ledger::Result::LEDGER_OK);
      return;
    }
  }

  CreateAnonAddress(callback);
}

void UpholdCard::CreateAnonAddress(ledger::ResultCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  const auto headers = RequestAuthorization(wallet->token);
  const std::string path = base::StringPrintf(
      "/v0/me/cards/%s/addresses",
      wallet->address.c_str());

  const std::string payload = base::StringPrintf(
      R"({
        "network": "%s"
      })",
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
    ledger::ResultCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  std::string id;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdCreateCard(response, &id);
  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(ledger::Result::EXPIRED_TOKEN);
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK || id.empty()) {
    BLOG(0, "Couldn't create anon address");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  wallet->anon_address = id;
  ledger_->ledger_client()->SaveExternalWallet(
      ledger::kWalletUphold,
      std::move(wallet));
  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_uphold
