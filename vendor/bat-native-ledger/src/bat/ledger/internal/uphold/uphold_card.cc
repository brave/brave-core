/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_card.h"

#include <utility>

#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/logging/logging.h"

namespace ledger::uphold {

UpholdCard::UpholdCard(LedgerImpl* ledger)
    : uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdCard::~UpholdCard() = default;

void UpholdCard::CreateBATCardIfNecessary(const std::string& access_token,
                                          CreateCardCallback callback) const {
  uphold_server_->get_cards()->Request(
      access_token,
      base::BindOnce(&UpholdCard::OnGetBATCardId, base::Unretained(this),
                     std::move(callback), access_token));
}

void UpholdCard::OnGetBATCardId(CreateCardCallback callback,
                                const std::string& access_token,
                                mojom::Result result,
                                std::string&& id) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result == mojom::Result::LEDGER_OK && !id.empty()) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK, std::move(id));
  }

  BLOG(1, "Couldn't get BAT card ID!");

  uphold_server_->post_cards()->Request(
      access_token,
      base::BindOnce(&UpholdCard::OnCreateBATCard, base::Unretained(this),
                     std::move(callback), access_token));
}

void UpholdCard::OnCreateBATCard(CreateCardCallback callback,
                                 const std::string& access_token,
                                 mojom::Result result,
                                 std::string&& id) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't create BAT card!");
    return std::move(callback).Run(result, "");
  }

  if (id.empty()) {
    BLOG(0, "BAT card ID is empty!");
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, "");
  }

  uphold_server_->patch_card()->Request(
      access_token, id,
      base::BindOnce(&UpholdCard::OnUpdateBATCardSettings,
                     base::Unretained(this), std::move(callback), id));
}

void UpholdCard::OnUpdateBATCardSettings(CreateCardCallback callback,
                                         std::string&& id,
                                         mojom::Result result) const {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't update BAT card settings!");
    return std::move(callback).Run(result, "");
  }

  DCHECK(!id.empty());
  std::move(callback).Run(mojom::Result::LEDGER_OK, std::move(id));
}

}  // namespace ledger::uphold
