/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_IN_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_IN_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_

#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/services/brave_wallet/public/cpp/third_party_service_launcher.h"

namespace brave_wallet {

class COMPONENT_EXPORT(BRAVE_WALLET_SERVICE) InProcessThirdPartyServiceLauncher
    : public ThirdPartyServiceLauncher {
 public:
  InProcessThirdPartyServiceLauncher();

  InProcessThirdPartyServiceLauncher(
      const InProcessThirdPartyServiceLauncher&) = delete;
  InProcessThirdPartyServiceLauncher& operator=(
      const InProcessThirdPartyServiceLauncher&) = delete;

  ~InProcessThirdPartyServiceLauncher() override;

  void Launch(
      mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
          receiver) override;

 private:
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_IN_PROCESS_THIRD_PARTY_SERVICE_LAUNCHER_H_
