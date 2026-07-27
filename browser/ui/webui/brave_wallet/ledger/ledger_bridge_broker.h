/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_BRIDGE_BROKER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_BRIDGE_BROKER_H_

#include "brave/components/brave_wallet/common/ledger_bridge.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// Owned by the trusted wallet page/panel WebUI controller. Bridges the
// untrusted `chrome-untrusted://ledger-bridge` frame (when running in mojo
// mode) and the trusted wallet page/panel renderer: it exposes
// `LedgerBridgeService` to the renderer (which
// registers a `LedgerBridgeListener`) and receives the child frame's
// `LedgerBridge` remote (via the controller's BindLedgerBridge()), forwarding
// it to the listener. If the child connects before the renderer registers its
// listener, the pending remote is buffered and flushed on registration.
class LedgerBridgeBroker : public mojom::LedgerBridgeService {
 public:
  LedgerBridgeBroker();
  LedgerBridgeBroker(const LedgerBridgeBroker&) = delete;
  LedgerBridgeBroker& operator=(const LedgerBridgeBroker&) = delete;
  ~LedgerBridgeBroker() override;

  // Binds the `LedgerBridgeService` requested by the trusted renderer.
  void BindService(mojo::PendingReceiver<mojom::LedgerBridgeService> receiver);

  // Called by the owning controller when the untrusted child frame hands up its
  // `LedgerBridge` remote.
  void OnChildBridgeConnected(mojo::PendingRemote<mojom::LedgerBridge> bridge);

 private:
  // mojom::LedgerBridgeService:
  void RegisterLedgerBridgeListener(
      mojo::PendingRemote<mojom::LedgerBridgeListener> listener) override;

  mojo::Receiver<mojom::LedgerBridgeService> service_receiver_{this};
  mojo::Remote<mojom::LedgerBridgeListener> listener_;
  // Buffers a child bridge that arrives before the listener is registered.
  mojo::PendingRemote<mojom::LedgerBridge> pending_bridge_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_LEDGER_LEDGER_BRIDGE_BROKER_H_
