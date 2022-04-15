/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/public/cpp/in_process_ledger_service.h"

#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/task/thread_pool.h"
#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace bat_ledger {

namespace {
void BindInProcessLedgerService(
    mojo::PendingReceiver<mojom::BatLedgerService> receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<BatLedgerServiceImpl>(),
                              std::move(receiver));
}

}  // namespace

namespace features {
const base::Feature kUseInProcessLedgerServiceFeature{
    "UseInProcessLedgerService", base::FEATURE_ENABLED_BY_DEFAULT};

bool UseInProcessLedgerService() {
  return base::FeatureList::IsEnabled(
      features::kUseInProcessLedgerServiceFeature);
}

}  // namespace features

void MakeInProcessLedgerService(
    mojo::PendingReceiver<mojom::BatLedgerService> receiver) {
  DCHECK(features::UseInProcessLedgerService());
  base::ThreadPool::CreateSequencedTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives()})
      ->PostTask(FROM_HERE, base::BindOnce(&BindInProcessLedgerService,
                                           std::move(receiver)));
}
}  // namespace bat_ledger
