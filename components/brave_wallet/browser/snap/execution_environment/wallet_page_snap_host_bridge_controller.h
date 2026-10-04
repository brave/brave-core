/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_HOST_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_HOST_BRIDGE_CONTROLLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// SnapHostBridgeController implementation that uses snap-host frames in an
// already-open wallet page. Does not open a page; IsBound() is true while a
// SnapHostBridge remote is bound (the page creates the pipe and passes the
// remote end).
//
// BindNewBridge is last-wins across the profile: a second wallet tab replaces
// the first tab's bridge remote, disconnecting its receiver; in-flight loads
// resolve as failures. Two further consequences, acceptable while snaps are
// behind a disabled-by-default flag:
//   - Closing the newer tab leaves IsBound() false even though the older tab
//     is still open, so loads fail until that tab is reloaded.
//   - The older tab keeps any snap iframes it already created; its orphaned
//     receiver never gets UnloadSnap.
// TODO(https://github.com/brave/brave-browser/issues/58686): track bridges
// per WebContents instead of one profile-wide remote.
class WalletPageSnapHostBridgeController : public SnapHostBridgeController {
 public:
  WalletPageSnapHostBridgeController();
  ~WalletPageSnapHostBridgeController() override;

  WalletPageSnapHostBridgeController(
      const WalletPageSnapHostBridgeController&) = delete;
  WalletPageSnapHostBridgeController& operator=(
      const WalletPageSnapHostBridgeController&) = delete;

  // SnapHostBridgeController:
  void BindNewBridge(
      mojo::PendingRemote<mojom::SnapHostBridge> bridge) override;
  bool IsBound() const override;
  void LoadSnap(const std::string& snap_id,
                const std::string& source_code,
                LoadSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id) override;

 private:
  void OnDisconnect();

  mojo::Remote<mojom::SnapHostBridge> snap_host_bridge_;

  base::WeakPtrFactory<WalletPageSnapHostBridgeController> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_WALLET_PAGE_SNAP_HOST_BRIDGE_CONTROLLER_H_
