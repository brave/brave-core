/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_wallet/ledger/ledger_bridge_broker.h"

#include <utility>

namespace brave_wallet {

LedgerBridgeBroker::LedgerBridgeBroker() = default;

LedgerBridgeBroker::~LedgerBridgeBroker() = default;

void LedgerBridgeBroker::BindService(
    mojo::PendingReceiver<mojom::LedgerBridgeService> receiver) {
  service_receiver_.reset();
  service_receiver_.Bind(std::move(receiver));
}

void LedgerBridgeBroker::OnChildBridgeConnected(
    mojo::PendingRemote<mojom::LedgerBridge> bridge) {
  if (listener_.is_bound()) {
    listener_->OnLedgerBridgeConnected(std::move(bridge));
    return;
  }
  // Listener not registered yet; keep the most recent bridge and flush it when
  // the renderer registers.
  pending_bridge_ = std::move(bridge);
}

void LedgerBridgeBroker::RegisterLedgerBridgeListener(
    mojo::PendingRemote<mojom::LedgerBridgeListener> listener) {
  listener_.reset();
  listener_.Bind(std::move(listener));
  if (pending_bridge_.is_valid()) {
    listener_->OnLedgerBridgeConnected(std::move(pending_bridge_));
    pending_bridge_.reset();
  }
}

}  // namespace brave_wallet
