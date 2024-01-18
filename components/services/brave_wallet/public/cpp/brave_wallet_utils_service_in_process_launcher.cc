/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_

#include "brave/components/services/brave_wallet/public/cpp/brave_wallet_utils_service_in_process_launcher.h"

#include <memory>
#include <utility>

#include "brave/components/services/brave_wallet/brave_wallet_utils_service_impl.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_wallet {

void LaunchInProcessBraveWalletUtilsService(
    mojo::PendingReceiver<mojom::BraveWalletUtilsService> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<BraveWalletUtilsServiceImpl>(
          mojo::PendingReceiver<mojom::BraveWalletUtilsService>()),
      std::move(receiver));
}

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_
