/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_account_chooser_contexts.h"

#include <utility>

namespace brave_wallet {

BraveWalletAccountChooserContexts::BraveWalletAccountChooserContexts(
    ContextMap contexts)
    : contexts_(std::move(contexts)) {}

BraveWalletAccountChooserContexts::~BraveWalletAccountChooserContexts() =
    default;

permissions::BraveWalletAccountChooserContext*
BraveWalletAccountChooserContexts::Get(mojom::CoinType coin) {
  const auto it = contexts_.find(coin);
  return it == contexts_.end() ? nullptr : it->second.get();
}

void BraveWalletAccountChooserContexts::Shutdown() {
  for (auto& [coin, context] : contexts_) {
    context->Shutdown();
  }
}

}  // namespace brave_wallet
