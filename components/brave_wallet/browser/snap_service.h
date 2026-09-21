/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SERVICE_H_

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {

class SnapHostBridgeController;

// Minimal browser-side service that exposes snap load to the wallet UI and
// owns the snap execution-environment bridge.
class SnapService : public mojom::SnapService {
 public:
  SnapService();
  ~SnapService() override;

  SnapService(const SnapService&) = delete;
  SnapService& operator=(const SnapService&) = delete;

  void Bind(mojo::PendingReceiver<mojom::SnapService> receiver);

  // mojom::SnapService:
  void LoadSnap(const std::string& snap_id, LoadSnapCallback callback) override;
  void BindSnapHostBridge(
      mojo::PendingRemote<mojom::SnapHostBridge> bridge) override;

  // Test seams.
  void SetSnapBundleForTesting(const std::string& snap_id,
                               const std::string& source_code);
  bool IsBridgeBoundForTesting() const;

 private:
  void OnLoadSnapResult(LoadSnapCallback callback,
                        bool success,
                        const std::optional<std::string>& error,
                        const std::optional<std::string>& result);

  std::unique_ptr<SnapHostBridgeController> bridge_controller_;

  // In-memory snap bundle store (snap_id -> source code).
  std::map<std::string, std::string> snap_bundles_;

  mojo::ReceiverSet<mojom::SnapService> receivers_;

  base::WeakPtrFactory<SnapService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_SERVICE_H_
