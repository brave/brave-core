/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/snap/execution_environment/snap_host_bridge_test_utils.h"

#include <optional>
#include <utility>

namespace brave_wallet {

FakeSnapHostBridge::FakeSnapHostBridge() = default;
FakeSnapHostBridge::~FakeSnapHostBridge() = default;

mojo::PendingRemote<mojom::SnapHostBridge>
FakeSnapHostBridge::BindNewPipeAndPassRemote() {
  return receiver_.BindNewPipeAndPassRemote();
}

void FakeSnapHostBridge::Reset() {
  receiver_.reset();
}

void FakeSnapHostBridge::LoadSnap(const std::string& snap_id,
                                  const std::string& source_code,
                                  LoadSnapCallback cb) {
  ++load_snap_call_count;
  last_snap_id = snap_id;
  last_source_code = source_code;
  if (next_load_succeeds) {
    std::move(cb).Run(true, std::nullopt, snap_id);
  } else {
    std::move(cb).Run(false, "load failed", std::nullopt);
  }
}

void FakeSnapHostBridge::UnloadSnap(const std::string& snap_id) {
  ++unload_snap_call_count;
}

FakeSnapHostBridgeController::FakeSnapHostBridgeController() = default;
FakeSnapHostBridgeController::~FakeSnapHostBridgeController() = default;

void FakeSnapHostBridgeController::BindNewBridge(
    mojo::PendingRemote<mojom::SnapHostBridge> bridge) {
  bound_ = true;
}

bool FakeSnapHostBridgeController::IsBound() const {
  return bound_;
}

void FakeSnapHostBridgeController::LoadSnap(const std::string& snap_id,
                                            const std::string& source_code,
                                            LoadSnapCallback cb) {
  ++load_snap_count;
  last_snap_id = snap_id;
  last_source_code = source_code;
  std::move(cb).Run(true, std::nullopt, load_snap_result);
}

void FakeSnapHostBridgeController::UnloadSnap(const std::string& snap_id) {}

}  // namespace brave_wallet
