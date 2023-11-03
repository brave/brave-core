/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_LAUNCHER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_LAUNCHER_H_

#include "base/component_export.h"
#include "brave/components/services/brave_wallet/public/mojom/third_party_service.mojom-forward.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_CPP) ThirdPartyServiceLauncher {
 public:
  ThirdPartyServiceLauncher();

  ThirdPartyServiceLauncher(const ThirdPartyServiceLauncher&) = delete;
  ThirdPartyServiceLauncher& operator=(const ThirdPartyServiceLauncher&) =
      delete;

  virtual ~ThirdPartyServiceLauncher();

  static ThirdPartyServiceLauncher* GetInstance();

  virtual void Launch(
      mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
          receiver) = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_THIRD_PARTY_SERVICE_LAUNCHER_H_
