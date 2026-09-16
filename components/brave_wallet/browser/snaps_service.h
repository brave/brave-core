/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_bridge_controller.h"
#include "brave/components/brave_wallet/browser/snap/snap_request_handler_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {

// Minimal browser-side service that exposes snap lifecycle operations to the
// wallet UI and owns the snap execution-environment bridge.
// This extraction intentionally omits manifest parsing, permission control,
// SES lockdown, installer/decompression, and persistent storage.
class SnapsService : public mojom::SnapsService {
 public:
  SnapsService();
  ~SnapsService() override;

  SnapsService(const SnapsService&) = delete;
  SnapsService& operator=(const SnapsService&) = delete;

  void Bind(mojo::PendingReceiver<mojom::SnapsService> receiver);

  // mojom::SnapsService:
  void InvokeSnap(const std::string& snap_id,
                  const std::string& method,
                  const std::string& params_json,
                  InvokeSnapCallback callback) override;

  void RequestInstallSnap(const std::string& snap_id,
                          const std::string& version,
                          RequestInstallSnapCallback callback) override;

  void GetSnapBundle(const std::string& snap_id,
                     GetSnapBundleCallback callback) override;

  // mojom::SnapsService — forwards to SnapBridgeController for the wallet
  // page WebUI setup.
  void SetSnapBridge(mojo::PendingRemote<mojom::SnapBridge> bridge) override;
  void BindSnapRequestHandler(
      mojo::PendingReceiver<mojom::SnapRequestHandler> receiver) override;

  // Test seams.
  void SetSnapBundleForTesting(const std::string& snap_id,
                               const std::string& source_code);
  bool IsBridgeBoundForTesting() const;
  SnapRequestHandlerImpl* snap_request_handler_for_testing() {
    return snap_request_handler_.get();
  }

 private:
  void OnBridgeReady(const std::string& snap_id,
                     const std::string& method,
                     base::Value params,
                     const std::string& caller_origin,
                     InvokeSnapCallback callback);
  void OnInvokeSnapResult(InvokeSnapCallback callback,
                          std::optional<base::Value> result,
                          const std::optional<std::string>& error);
  void OnLoadSnapResult(InvokeSnapCallback callback,
                        const std::string& snap_id,
                        const std::string& method,
                        base::Value params,
                        const std::string& caller_origin,
                        bool success,
                        const std::optional<std::string>& error);

  std::unique_ptr<SnapBridgeController> bridge_controller_;
  std::unique_ptr<SnapRequestHandlerImpl> snap_request_handler_;

  // In-memory snap bundle store (snap_id -> source code).
  std::map<std::string, std::string> snap_bundles_;

  mojo::ReceiverSet<mojom::SnapsService> receivers_;

  base::WeakPtrFactory<SnapsService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAPS_SERVICE_H_
