/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// SnapBridgeController implementation that uses the wallet page as the snap
// isolation environment. When the bridge is not bound, opens the wallet page
// via OpenWalletPageCallback so the iframe loads and reconnects.
class WalletPageSnapBridgeController : public SnapBridgeController {
 public:
  using OpenWalletPageCallback = base::RepeatingClosure;

  explicit WalletPageSnapBridgeController(
      OpenWalletPageCallback open_wallet_page);
  ~WalletPageSnapBridgeController() override;

  WalletPageSnapBridgeController(const WalletPageSnapBridgeController&) =
      delete;
  WalletPageSnapBridgeController& operator=(
      const WalletPageSnapBridgeController&) = delete;

  // SnapBridgeController:
  void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) override;
  bool IsBound() const override;
  void SetDisconnectCallback(DisconnectCallback cb) override;
  void EnsureBridgeReady(base::OnceClosure on_ready) override;
  void LoadSnap(const std::string& snap_id, LoadSnapCallback cb) override;
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  base::Value params,
                  const std::string& caller_origin,
                  InvokeSnapCallback cb) override;
  void FetchSnapHomePage(const std::string& snap_id,
                         FetchSnapHomePageCallback cb) override;
  void SendSnapUserInputEvent(const std::string& snap_id,
                              const std::string& interface_id,
                              const std::string& event_json,
                              SendSnapUserInputEventCallback cb) override;

 private:
  void OnDisconnect();
  void DrainReadyCallbacks();

  OpenWalletPageCallback open_wallet_page_;
  DisconnectCallback disconnect_callback_;

  mojo::Remote<mojom::SnapBridge> snap_bridge_;
  std::vector<base::OnceClosure> pending_ready_callbacks_;
  bool bridge_open_inflight_ = false;

  base::WeakPtrFactory<WalletPageSnapBridgeController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_
