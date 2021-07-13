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
#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace uphold {

UpdateCard::UpdateCard() :
  label(""),
  position(-1),
  starred(false) {}

UpdateCard::~UpdateCard() = default;

UpholdCard::UpholdCard(LedgerImpl* ledger) :
    ledger_(ledger),
    uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {
}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateIfNecessary(CreateCardCallback callback) {
  auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto url_callback = std::bind(&UpholdCard::OnCreateIfNecessary,
      this,
      _1,
      _2,
      callback);

  uphold_server_->get_cards()->Request(wallet->token, url_callback);
}

void UpholdCard::OnCreateIfNecessary(
    const type::Result result,
    const std::string& id,
    CreateCardCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    callback(type::Result::EXPIRED_TOKEN, "");
    ledger_->uphold()->DisconnectWallet();
    return;
  }

  if (result == type::Result::LEDGER_OK && !id.empty()) {
    callback(type::Result::LEDGER_OK, id);
    return;
  }

  Create(callback);
}

void UpholdCard::Create(
    CreateCardCallback callback) {
  auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto url_callback = std::bind(&UpholdCard::OnCreate,
      this,
      _1,
      _2,
      callback);

  uphold_server_->post_cards()->Request(wallet->token, url_callback);
}

void UpholdCard::OnCreate(
    const type::Result result,
    const std::string& id,
    CreateCardCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN, "");
    ledger_->uphold()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK || id.empty()) {
    BLOG(0, "Couldn't create anon card address");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto wallet_ptr = ledger_->uphold()->GetWallet();
  if (!wallet_ptr) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }
  wallet_ptr->address = id;
  ledger_->uphold()->SetWallet(wallet_ptr->Clone());

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
    const type::Result result,
    const std::string& address,
    CreateCardCallback callback) {
  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Card update failed");
    callback(result, "");
    return;
  }

  callback(result, address);
}

void UpholdCard::Update(
    const UpdateCard& card,
    ledger::ResultCallback callback) {
  auto wallet = ledger_->uphold()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto url_callback = std::bind(&UpholdCard::OnUpdate,
      this,
      _1,
      callback);

  uphold_server_->patch_card()->Request(
      wallet->token,
      wallet->address,
      card,
      url_callback);
}

void UpholdCard::OnUpdate(
    const type::Result result,
    ledger::ResultCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN);
    ledger_->uphold()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't update rewards card");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  callback(type::Result::LEDGER_OK);
}

}  // namespace uphold
}  // namespace ledger
