/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_TEST_UTILS_H_

#include <string>

#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

// Records mojom::SnapHostBridge calls and answers LoadSnap with |snap_id| as
// the result, matching the real host's behavior for a bundle whose export is
// a string.
class FakeSnapHostBridge : public mojom::SnapHostBridge {
 public:
  FakeSnapHostBridge();
  ~FakeSnapHostBridge() override;

  mojo::PendingRemote<mojom::SnapHostBridge> BindNewPipeAndPassRemote();
  // Closes the pipe to simulate the page going away.
  void Reset();

  // mojom::SnapHostBridge:
  void LoadSnap(const std::string& snap_id,
                const std::string& source_code,
                LoadSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id) override;

  int load_snap_call_count = 0;
  int unload_snap_call_count = 0;
  std::string last_snap_id;
  std::string last_source_code;
  bool next_load_succeeds = true;

 private:
  mojo::Receiver<mojom::SnapHostBridge> receiver_{this};
};

// SnapHostBridgeController stub for SnapService tests. EnsureBridgeReady runs
// |on_ready| immediately unless set_defer_ready(true); RunPendingReady()
// releases a deferred callback.
class FakeSnapHostBridgeController : public SnapHostBridgeController {
 public:
  FakeSnapHostBridgeController();
  ~FakeSnapHostBridgeController() override;

  // SnapHostBridgeController:
  void BindNewBridge(
      mojo::PendingRemote<mojom::SnapHostBridge> bridge) override;
  bool IsBound() const override;
  void EnsureBridgeReady(base::OnceClosure on_ready) override;
  void LoadSnap(const std::string& snap_id,
                const std::string& source_code,
                LoadSnapCallback cb) override;
  void UnloadSnap(const std::string& snap_id) override;
  void Shutdown() override;
  std::string GetUnavailableError() const override;

  void RunPendingReady();
  void set_defer_ready(bool defer) { defer_ready_ = defer; }
  void set_bound(bool bound) { bound_ = bound; }

  int ensure_bridge_ready_count = 0;
  int shutdown_count = 0;
  int load_snap_count = 0;
  std::string last_snap_id;
  std::string last_source_code;
  std::string load_snap_result;

 private:
  bool bound_ = false;
  bool defer_ready_ = false;
  base::OnceClosure pending_ready_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SNAP_EXECUTION_ENVIRONMENT_SNAP_HOST_BRIDGE_TEST_UTILS_H_
