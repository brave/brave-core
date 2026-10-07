/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_CONTROLLER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

// Abstract interface for managing the mojo::Remote<mojom::SnapHostBridge>
// and routing snap load operations to the wallet page.
class SnapHostBridgeController {
 public:
  using LoadSnapCallback =
      base::OnceCallback<void(bool,
                              const std::optional<std::string>&,
                              const std::optional<std::string>&)>;

  virtual ~SnapHostBridgeController() = default;

  SnapHostBridgeController(const SnapHostBridgeController&) = delete;
  SnapHostBridgeController& operator=(const SnapHostBridgeController&) = delete;

  // Binds a new SnapHostBridge remote (last-wins across the profile). The
  // wallet page creates the pipe and passes the remote end.
  virtual void BindNewBridge(
      mojo::PendingRemote<mojom::SnapHostBridge> bridge) = 0;

  virtual bool IsBound() const = 0;

  // Forwards to the bound bridge. LoadSnap reports a disconnection and
  // UnloadSnap is a no-op when no bridge is bound.
  virtual void LoadSnap(const std::string& snap_id,
                        const std::string& source_code,
                        LoadSnapCallback cb) = 0;
  virtual void UnloadSnap(const std::string& snap_id) = 0;

 protected:
  SnapHostBridgeController() = default;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_CONTROLLER_H_
