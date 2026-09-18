/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace brave_wallet {

// Abstract interface for managing the mojo::Remote<mojom::SnapBridge>
// and routing snap load operations to the wallet page.
class SnapBridgeController {
 public:
  using LoadSnapCallback =
      base::OnceCallback<void(bool,
                              const std::optional<std::string>&,
                              const std::optional<std::string>&)>;

  virtual ~SnapBridgeController() = default;

  SnapBridgeController(const SnapBridgeController&) = delete;
  SnapBridgeController& operator=(const SnapBridgeController&) = delete;

  // Called by the wallet page to bind the bridge. Last-wins across the
  // profile: a second wallet tab replaces the first tab's bridge.
  virtual void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) = 0;

  virtual bool IsBound() const = 0;

  // mojom::SnapBridge passthroughs — bridge must be bound when called.
  virtual void LoadSnap(const std::string& snap_id,
                        const std::string& source_code,
                        LoadSnapCallback cb) = 0;
  virtual void UnloadSnap(const std::string& snap_id) = 0;

 protected:
  SnapBridgeController() = default;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_
