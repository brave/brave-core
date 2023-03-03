/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_

#include <memory>

#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_card/get_card.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_cards/get_cards.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/patch_card/patch_card.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/post_cards/post_cards.h"
#include "brave/components/brave_rewards/core/ledger.h"

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

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_UPHOLD_UPHOLD_SERVER_H_
