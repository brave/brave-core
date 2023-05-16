/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/ledger_factory_impl.h"

#include <memory>
#include <utility>

#include "base/task/bind_post_task.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"

namespace brave_rewards::internal {

LedgerFactoryImpl::LedgerFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {
  VLOG(0) << "Constructor";
}

LedgerFactoryImpl::~LedgerFactoryImpl() {
  VLOG(0) << "Destructor";
}

void LedgerFactoryImpl::CreateLedger(
    const base::FilePath& profile,
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    CreateLedgerCallback callback) {
  auto ledger_task_runner = base::ThreadPool::CreateSingleThreadTaskRunner(
      {base::MayBlock(), base::WithBaseSyncPrimitives(),
       base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::SingleThreadTaskRunnerThreadMode::DEDICATED);

  ledger_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(
          &LedgerFactoryImpl::CreateLedgerOnTaskRunner, base::Unretained(this),
          profile, std::move(ledger_receiver), std::move(ledger_client_remote),
          base::SingleThreadTaskRunner::GetCurrentDefault(),
          ledger_task_runner),
      base::BindOnce(&LedgerFactoryImpl::AddProfile, base::Unretained(this),
                     profile, std::move(callback)));
}

class SelfOwnedLedger {
 public:
  static void Create(
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::OnceClosure disconnect_handler) {
    new SelfOwnedLedger(std::move(ledger_client_remote),
                        std::move(ledger_receiver), std::move(task_runner),
                        std::move(disconnect_handler));
  }

  SelfOwnedLedger(const SelfOwnedLedger&) = delete;
  SelfOwnedLedger& operator=(const SelfOwnedLedger&) = delete;

 private:
  void Delete() { delete this; }

  SelfOwnedLedger(
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::OnceClosure disconnect_handler)
      : ledger_client_remote_(std::move(ledger_client_remote), task_runner),
        ledger_(std::make_unique<LedgerImpl>(ledger_client_remote_.get())),
        ledger_receiver_(&ledger(ledger_.get()),
                         std::move(ledger_receiver),
                         task_runner) {
    ledger_receiver_.set_disconnect_handler(
        base::BindOnce(&SelfOwnedLedger::Delete, base::Unretained(this))
            .Then(std::move(disconnect_handler)));
  }

  ~SelfOwnedLedger() = default;

  mojo::AssociatedRemote<mojom::LedgerClient> ledger_client_remote_;
  std::unique_ptr<LedgerImpl> ledger_;
  mojo::AssociatedReceiver<mojom::Ledger> ledger_receiver_;
};

void LedgerFactoryImpl::CreateLedgerOnTaskRunner(
    base::FilePath&& profile,
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> ledger_task_runner) {
  VLOG(0) << "Creating ledger for " << profile << "...";

  SelfOwnedLedger::Create(
      std::move(ledger_client_remote), std::move(ledger_receiver),
      std::move(ledger_task_runner),
      base::BindPostTask(
          main_task_runner,
          base::BindOnce(&LedgerFactoryImpl::RemoveProfile,
                         base::Unretained(this), std::move(profile))));
}

void LedgerFactoryImpl::AddProfile(base::FilePath&& profile,
                                   CreateLedgerCallback callback) {
  const auto [profile_it, added] =
      profiles_with_ledger_.insert(std::move(profile));
  DCHECK(added);

  VLOG(0) << "Added ledger for " << *profile_it;
  VLOG(0) << "Number of ledgers: " << profiles_with_ledger_.size();

  std::move(callback).Run();
}

void LedgerFactoryImpl::RemoveProfile(const base::FilePath& profile) {
  const auto removed = profiles_with_ledger_.erase(profile);
  DCHECK(removed);

  VLOG(0) << "Removed ledger for " << profile;
  VLOG(0) << "Number of ledgers: " << profiles_with_ledger_.size();

  if (profiles_with_ledger_.empty()) {
    receiver_.reset();
  }
}

}  // namespace brave_rewards::internal
