/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/content/out_of_process_third_party_service_launcher.h"

#include <utility>

#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom.h"
#include "content/public/browser/service_process_host.h"

namespace brave_wallet {

OutOfProcessThirdPartyServiceLauncher::OutOfProcessThirdPartyServiceLauncher() =
    default;

OutOfProcessThirdPartyServiceLauncher::
    ~OutOfProcessThirdPartyServiceLauncher() = default;

void OutOfProcessThirdPartyServiceLauncher::Launch(
    mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
        receiver) {
  content::ServiceProcessHost::Launch(
      std::move(receiver),
      content::ServiceProcessHost::Options()
          .WithDisplayName("Brave Wallet Third Party Service")
          .Pass());
}

}  // namespace brave_wallet
