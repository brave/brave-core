/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::core {
namespace endpoint {

UpholdServer::UpholdServer(LedgerImpl* ledger)
    : get_capabilities_(std::make_unique<uphold::GetCapabilities>(ledger)),
      get_cards_(std::make_unique<uphold::GetCards>(ledger)),
      get_card_(std::make_unique<uphold::GetCard>(ledger)),
      get_me_(std::make_unique<uphold::GetMe>(ledger)),
      post_cards_(std::make_unique<uphold::PostCards>(ledger)),
      patch_card_(std::make_unique<uphold::PatchCard>(ledger)) {}

UpholdServer::~UpholdServer() = default;

uphold::GetCapabilities* UpholdServer::get_capabilities() const {
  return get_capabilities_.get();
}

uphold::GetCards* UpholdServer::get_cards() const {
  return get_cards_.get();
}

uphold::GetCard* UpholdServer::get_card() const {
  return get_card_.get();
}

uphold::GetMe* UpholdServer::get_me() const {
  return get_me_.get();
}

uphold::PostCards* UpholdServer::post_cards() const {
  return post_cards_.get();
}

uphold::PatchCard* UpholdServer::patch_card() const {
  return patch_card_.get();
}

}  // namespace endpoint
}  // namespace brave_rewards::core
