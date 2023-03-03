/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_

#include <memory>

#include "bat/ledger/internal/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "bat/ledger/internal/endpoint/uphold/get_card/get_card.h"
#include "bat/ledger/internal/endpoint/uphold/get_cards/get_cards.h"
#include "bat/ledger/internal/endpoint/uphold/get_me/get_me.h"
#include "bat/ledger/internal/endpoint/uphold/patch_card/patch_card.h"
#include "bat/ledger/internal/endpoint/uphold/post_cards/post_cards.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {

class UpholdServer {
 public:
  explicit UpholdServer(LedgerImpl*);
  ~UpholdServer();

  uphold::GetCapabilities* get_capabilities() const;

  uphold::GetCards* get_cards() const;

  uphold::GetCard* get_card() const;

  uphold::GetMe* get_me() const;

  uphold::PostCards* post_cards() const;

  uphold::PatchCard* patch_card() const;

 private:
  std::unique_ptr<uphold::GetCapabilities> get_capabilities_;
  std::unique_ptr<uphold::GetCards> get_cards_;
  std::unique_ptr<uphold::GetCard> get_card_;
  std::unique_ptr<uphold::GetMe> get_me_;
  std::unique_ptr<uphold::PostCards> post_cards_;
  std::unique_ptr<uphold::PatchCard> patch_card_;
};

}  // namespace endpoint
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_
