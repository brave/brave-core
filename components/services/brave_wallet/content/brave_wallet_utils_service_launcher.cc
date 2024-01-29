/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/content/brave_wallet_utils_service_launcher.h"

#include <utility>

#include "brave/grit/brave_generated_resources.h"
#include "content/public/browser/service_process_host.h"

namespace brave_wallet {

void LaunchBraveWalletUtilsService(
    mojo::PendingReceiver<mojom::BraveWalletUtilsService> receiver) {
  content::ServiceProcessHost::Launch(
      std::move(receiver),
      content::ServiceProcessHost::Options()
          .WithDisplayName(IDS_UTILITY_PROCESS_WALLET_UTILS_NAME)
          .Pass());
}

}  // namespace brave_wallet
