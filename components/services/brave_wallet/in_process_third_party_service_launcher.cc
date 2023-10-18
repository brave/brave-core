/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/in_process_third_party_service_launcher.h"

#include <memory>
#include <utility>

#include "base/task/thread_pool.h"
#include "brave/components/services/brave_wallet/third_party_service_impl.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace brave_wallet {

namespace {

void BindInProcessThirdPartyService(
    mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
        receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<ThirdPartyServiceImpl>(),
                              std::move(receiver));
}

}  // namespace

InProcessThirdPartyServiceLauncher::InProcessThirdPartyServiceLauncher()
    : task_runner_(base::ThreadPool::CreateSequencedTaskRunner({})) {}

InProcessThirdPartyServiceLauncher::~InProcessThirdPartyServiceLauncher() =
    default;

void InProcessThirdPartyServiceLauncher::Launch(
    mojo::PendingReceiver<third_party_service::mojom::ThirdPartyService>
        receiver) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BindInProcessThirdPartyService, std::move(receiver)));
}

}  // namespace brave_wallet
