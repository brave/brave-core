/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/ledger_factory_impl.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/task/bind_post_task.h"
#include "brave/components/brave_rewards/core/self_owned_ledger_receiver.h"

namespace brave_rewards::internal {

LedgerFactoryImpl::LedgerFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {
  VLOG(0) << "LedgerFactoryImpl()";
}

LedgerFactoryImpl::~LedgerFactoryImpl() {
  VLOG(0) << "~LedgerFactoryImpl()";
}

void LedgerFactoryImpl::CreateLedger(
    const base::FilePath& profile,
    mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
    CreateLedgerCallback callback) {
  const auto [entry, inserted] = ledger_threads_.emplace(profile, "szilard");
  DCHECK(inserted);
  if (!inserted) {
    return std::move(callback).Run();
  }

  base::Thread& thread = entry->second;
  thread.Start();
  thread.task_runner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          // SelfOwnedLedgerReceiver::Create runs on the ledger's task runner
          &SelfOwnedLedgerReceiver::Create, std::move(remote),
          std::move(receiver),
          base::BindPostTask(
              base::SingleThreadTaskRunner::GetCurrentDefault(),
              base::BindOnce(
                  // LedgerRemovedCallback runs on the main task runner
                  &LedgerFactoryImpl::LedgerRemovedCallback,
                  base::Unretained(this), profile))),
      base::BindOnce(
          // LedgerAddedCallback runs on the main task runner
          &LedgerFactoryImpl::LedgerAddedCallback, base::Unretained(this),
          profile, std::move(callback)));
}

void LedgerFactoryImpl::LedgerAddedCallback(const base::FilePath& profile,
                                            CreateLedgerCallback callback) {
  VLOG(0) << "Added ledger for " << profile;
  VLOG(0) << "Number of ledgers: " << ledger_threads_.size();

  std::move(callback).Run();
}

void LedgerFactoryImpl::LedgerRemovedCallback(const base::FilePath& profile) {
  ledger_threads_.erase(profile);

  VLOG(0) << "Removed ledger for " << profile;
  VLOG(0) << "Number of ledgers: " << ledger_threads_.size();

  if (ledger_threads_.empty()) {
    receiver_.reset();
  }
}

}  // namespace brave_rewards::internal
