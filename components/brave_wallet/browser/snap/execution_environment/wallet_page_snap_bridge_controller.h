/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// SnapBridgeController implementation that uses an already-open wallet page
// as the snap isolation environment. Does not open a page; IsBound() is true
// only while a wallet page has connected the bridge.
//
// SetBridge is last-wins across the profile: a second wallet tab replaces
// the first tab's bridge remote.
class WalletPageSnapBridgeController : public SnapBridgeController {
 public:
  WalletPageSnapBridgeController();
  ~WalletPageSnapBridgeController() override;

  WalletPageSnapBridgeController(const WalletPageSnapBridgeController&) =
      delete;
  WalletPageSnapBridgeController& operator=(
      const WalletPageSnapBridgeController&) = delete;

  // SnapBridgeController:
  void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) override;
  bool IsBound() const override;
  void LoadSnap(const std::string& snap_id,
                const std::string& source_code,
                LoadSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id) override;

 private:
  void OnDisconnect();

  mojo::Remote<mojom::SnapBridge> snap_bridge_;

  base::WeakPtrFactory<WalletPageSnapBridgeController> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_BRIDGE_CONTROLLER_H_
