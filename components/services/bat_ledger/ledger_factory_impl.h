/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_

#include <set>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"
#include "brave/components/services/bat_ledger/public/interfaces/ledger_factory.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_rewards::internal {

class LedgerFactoryImpl : public mojom::LedgerFactory {
 public:
  explicit LedgerFactoryImpl(
      mojo::PendingReceiver<mojom::LedgerFactory> receiver);

  ~LedgerFactoryImpl() override;

  LedgerFactoryImpl(const LedgerFactoryImpl&) = delete;
  LedgerFactoryImpl& operator=(const LedgerFactoryImpl&) = delete;

  void CreateLedger(
      const base::FilePath& profile,
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
      CreateLedgerCallback callback) override;

 private:
  void CreateLedgerOnTaskRunner(
      base::FilePath&& profile,
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> ledger_task_runner);

  void AddProfile(base::FilePath&& profile, CreateLedgerCallback callback);

  void RemoveProfile(const base::FilePath& profile);

  mojo::Receiver<mojom::LedgerFactory> receiver_;
  std::set<base::FilePath> profiles_with_ledger_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_
