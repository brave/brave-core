/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/endpoint/uphold/get_cards/get_cards.h"
#include "bat/ledger/internal/endpoint/uphold/patch_card/patch_card.h"
#include "bat/ledger/internal/endpoint/uphold/post_cards/post_cards.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class UpholdServer;
}

namespace uphold {

const char kCardName[] = "Brave Browser";

class UpholdCard {
 public:
  explicit UpholdCard(LedgerImpl* ledger);

  ~UpholdCard();

  void CreateBATCardIfNecessary(CreateCardCallback callback) const;

 private:
  void GetBATCardId(endpoint::uphold::GetCardsCallback callback) const;

  void OnGetBATCardId(const type::Result result,
                      const std::string& id,
                      CreateCardCallback callback) const;

  void CreateBATCard(endpoint::uphold::PostCardsCallback callback) const;

  void OnCreateBATCard(const type::Result result,
                       const std::string& id,
                       CreateCardCallback callback) const;

  void UpdateBATCardSettings(
      const std::string& id,
      endpoint::uphold::PatchCardCallback callback) const;

  void OnUpdateBATCardSettings(const type::Result result,
                               const std::string& id,
                               CreateCardCallback callback) const;

  LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<endpoint::UpholdServer> uphold_server_;
};

}  // namespace uphold
}  // namespace ledger
#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_CARD_H_
