/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_card.h"

#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

UpholdCard::UpholdCard(LedgerImpl* ledger)
    : ledger_(ledger),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateBATCardIfNecessary(CreateCardCallback callback) const {
  GetBATCardId(std::bind(&UpholdCard::OnGetBATCardId, this, _1, _2, callback));
}

void UpholdCard::GetBATCardId(
    endpoint::uphold::GetCardsCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  uphold_server_->get_cards()->Request(uphold_wallet->token, callback);
}

void UpholdCard::OnGetBATCardId(const type::Result result,
                                const std::string& id,
                                CreateCardCallback callback) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return callback(type::Result::EXPIRED_TOKEN, "");
  }

  if (result == type::Result::LEDGER_OK && !id.empty()) {
    return callback(type::Result::LEDGER_OK, id);
  }

  BLOG(1, "Couldn't get BAT card ID!");

  CreateBATCard(
      std::bind(&UpholdCard::OnCreateBATCard, this, _1, _2, callback));
}

void UpholdCard::CreateBATCard(
    endpoint::uphold::PostCardsCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  uphold_server_->post_cards()->Request(uphold_wallet->token, callback);
}

void UpholdCard::OnCreateBATCard(const type::Result result,
                                 const std::string& id,
                                 CreateCardCallback callback) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return callback(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't create BAT card!");
    return callback(result, "");
  }

  if (id.empty()) {
    BLOG(0, "BAT card ID is empty!");
    return callback(type::Result::LEDGER_ERROR, "");
  }

  UpdateBATCardSettings(id, std::bind(&UpholdCard::OnUpdateBATCardSettings,
                                      this, _1, id, callback));
}

void UpholdCard::UpdateBATCardSettings(
    const std::string& id,
    endpoint::uphold::PatchCardCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR);
  }

  DCHECK(!id.empty());
  uphold_server_->patch_card()->Request(uphold_wallet->token, id, callback);
}

void UpholdCard::OnUpdateBATCardSettings(const type::Result result,
                                         const std::string& id,
                                         CreateCardCallback callback) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return callback(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't update BAT card settings!");
    return callback(result, "");
  }

  DCHECK(!id.empty());
  callback(type::Result::LEDGER_OK, id);
}

}  // namespace uphold
}  // namespace ledger
