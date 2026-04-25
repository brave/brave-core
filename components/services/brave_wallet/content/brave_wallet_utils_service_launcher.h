/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_

#include "mojo/public/cpp/bindings/pending_receiver.h"

#include "brave/components/services/brave_wallet/public/mojom/brave_wallet_utils_service.mojom.h"

namespace brave_wallet {

void LaunchBraveWalletUtilsService(
    mojo::PendingReceiver<mojom::BraveWalletUtilsService> receiver);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_BRAVE_WALLET_UTILS_SERVICE_LAUNCHER_H_
