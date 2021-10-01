/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"

#include <memory>
#include <string>
#include <utility>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

TrezorBridgeController::TrezorBridgeController() {}

TrezorBridgeController::~TrezorBridgeController() = default;

mojo::PendingRemote<mojom::TrezorBridgeController>
TrezorBridgeController::MakeRemote() {
  mojo::PendingRemote<mojom::TrezorBridgeController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void TrezorBridgeController::Bind(
    mojo::PendingReceiver<mojom::TrezorBridgeController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void TrezorBridgeController::Unlock(UnlockCallback callback) {
  DLOG(INFO) << "TrezorBridgeController: AAAAAa";
  std::move(callback).Run(true);
}

void TrezorBridgeController::GetAddress(const std::string& path,
                                        GetAddressCallback callback) {
  std::move(callback).Run(true, path);
}

}  // namespace brave_wallet
