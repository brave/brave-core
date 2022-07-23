/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_card.h"

#include <utility>

#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

namespace ledger {
namespace uphold {

UpholdCard::UpholdCard(LedgerImpl* ledger)
    : ledger_(ledger),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateBATCardIfNecessary(CreateCardCallback callback) const {
  GetBATCardId(base::BindOnce(&UpholdCard::OnGetBATCardId,
                              base::Unretained(this), std::move(callback)));
}

void UpholdCard::GetBATCardId(
    endpoint::uphold::GetCardsCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  uphold_server_->get_cards()->Request(uphold_wallet->token,
                                       std::move(callback));
}

void UpholdCard::OnGetBATCardId(CreateCardCallback callback,
                                type::Result result,
                                const std::string& id) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN, "");
  }

  if (result == type::Result::LEDGER_OK && !id.empty()) {
    return std::move(callback).Run(type::Result::LEDGER_OK, id);
  }

  BLOG(1, "Couldn't get BAT card ID!");

  CreateBATCard(base::BindOnce(&UpholdCard::OnCreateBATCard,
                               base::Unretained(this), std::move(callback)));
}

void UpholdCard::CreateBATCard(
    endpoint::uphold::PostCardsCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  uphold_server_->post_cards()->Request(uphold_wallet->token,
                                        std::move(callback));
}

void UpholdCard::OnCreateBATCard(CreateCardCallback callback,
                                 type::Result result,
                                 const std::string& id) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't create BAT card!");
    return std::move(callback).Run(result, "");
  }

  if (id.empty()) {
    BLOG(0, "BAT card ID is empty!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR, "");
  }

  UpdateBATCardSettings(
      id, base::BindOnce(&UpholdCard::OnUpdateBATCardSettings,
                         base::Unretained(this), std::move(callback), id));
}

void UpholdCard::UpdateBATCardSettings(
    const std::string& id,
    endpoint::uphold::PatchCardCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  DCHECK(!id.empty());
  uphold_server_->patch_card()->Request(uphold_wallet->token, id,
                                        std::move(callback));
}

void UpholdCard::OnUpdateBATCardSettings(CreateCardCallback callback,
                                         const std::string& id,
                                         type::Result result) const {
  if (result == type::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(type::Result::EXPIRED_TOKEN, "");
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't update BAT card settings!");
    return std::move(callback).Run(result, "");
  }

  DCHECK(!id.empty());
  std::move(callback).Run(type::Result::LEDGER_OK, id);
}

}  // namespace uphold
}  // namespace ledger
