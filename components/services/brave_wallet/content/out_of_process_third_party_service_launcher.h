/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_OUT_OF_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_OUT_OF_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_

#include "brave/components/services/brave_wallet/public/cpp/third_party_service_launcher.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_SERVICE_CONTENT)
    OutOfProcessThirdPartyServiceLauncher : public ThirdPartyServiceLauncher {
 public:
  OutOfProcessThirdPartyServiceLauncher();

  OutOfProcessThirdPartyServiceLauncher(
      const OutOfProcessThirdPartyServiceLauncher&) = delete;
  OutOfProcessThirdPartyServiceLauncher& operator=(
      const OutOfProcessThirdPartyServiceLauncher&) = delete;

  ~OutOfProcessThirdPartyServiceLauncher() override;

  void Launch(
      mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
          receiver) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_CONTENT_OUT_OF_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_
