/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// Abstract interface for managing the mojo::Remote<mojom::SnapBridge>
// and routing snap invocations to the renderer host (wallet page or
// hidden WebContents).
class SnapBridgeController {
 public:
  using DisconnectCallback = base::RepeatingClosure;

  using LoadSnapCallback =
      base::OnceCallback<void(bool, const std::optional<std::string>&)>;
  using InvokeSnapCallback =
      base::OnceCallback<void(std::optional<base::Value>,
                              const std::optional<std::string>&)>;
  using FetchSnapHomePageCallback =
      base::OnceCallback<void(const std::optional<std::string>&,
                              const std::optional<std::string>&,
                              const std::optional<std::string>&)>;
  using SendSnapUserInputEventCallback =
      base::OnceCallback<void(const std::optional<std::string>&,
                              const std::optional<std::string>&)>;

  virtual ~SnapBridgeController() = default;

  SnapBridgeController(const SnapBridgeController&) = delete;
  SnapBridgeController& operator=(const SnapBridgeController&) = delete;

  // Called by both the wallet page and the hidden host page to bind the bridge.
  virtual void SetBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) = 0;

  virtual bool IsBound() const = 0;

  // Registers a callback fired on every bridge disconnect.
  virtual void SetDisconnectCallback(DisconnectCallback cb) = 0;

  // Ensures the bridge is ready, then runs |on_ready|.
  virtual void EnsureBridgeReady(base::OnceClosure on_ready) = 0;

  // mojom::SnapBridge passthroughs — bridge must be bound when called.
  virtual void LoadSnap(const std::string& snap_id, LoadSnapCallback cb) = 0;
  virtual void InvokeSnap(const std::string& snap_id,
                          const std::string& method,
                          base::Value params,
                          const std::string& caller_origin,
                          InvokeSnapCallback cb) = 0;
  virtual void FetchSnapHomePage(const std::string& snap_id,
                                 FetchSnapHomePageCallback cb) = 0;
  virtual void SendSnapUserInputEvent(const std::string& snap_id,
                                      const std::string& interface_id,
                                      const std::string& event_json,
                                      SendSnapUserInputEventCallback cb) = 0;

 protected:
  SnapBridgeController() = default;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_BRIDGE_CONTROLLER_H_
