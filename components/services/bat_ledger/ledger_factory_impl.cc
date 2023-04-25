/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/ledger_factory_impl.h"

#include <memory>
#include <utility>

#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {

LedgerFactoryImpl::LedgerFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {}

LedgerFactoryImpl::~LedgerFactoryImpl() = default;

void LedgerFactoryImpl::CreateLedger(
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    CreateLedgerCallback callback) {
  if (!ledger_) {
    auto task_runner = base::ThreadPool::CreateSingleThreadTaskRunner(
        {base::MayBlock(), base::WithBaseSyncPrimitives(),
         base::TaskPriority::USER_BLOCKING,
         base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
        base::SingleThreadTaskRunnerThreadMode::DEDICATED);

    task_runner->PostTaskAndReplyWithResult(
        FROM_HERE,
        base::BindOnce(&LedgerFactoryImpl::CreateLedgerOnDedicatedTaskRunner,
                       base::Unretained(this), std::move(ledger_receiver),
                       std::move(ledger_client_remote), task_runner),
        base::BindOnce(
            &LedgerFactoryImpl::CreateLedgerOnDedicatedTaskRunnerCallback,
            base::Unretained(this), std::move(callback)));
  }
}

mojo::SelfOwnedAssociatedReceiverRef<mojom::Ledger>
LedgerFactoryImpl::CreateLedgerOnDedicatedTaskRunner(
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  return mojo::MakeSelfOwnedAssociatedReceiver(
      std::make_unique<LedgerImpl>(std::move(ledger_client_remote)),
      std::move(ledger_receiver), std::move(task_runner));
}

void LedgerFactoryImpl::CreateLedgerOnDedicatedTaskRunnerCallback(
    CreateLedgerCallback callback,
    mojo::SelfOwnedAssociatedReceiverRef<mojom::Ledger> ledger) {
  ledger_ = ledger;

  std::move(callback).Run();
}

}  // namespace ledger
