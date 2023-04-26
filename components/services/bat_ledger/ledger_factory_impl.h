/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_

#include <map>

#include "base/files/file_path.h"
#include "base/functional/callback_forward.h"
#include "base/threading/thread.h"
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

  LedgerFactoryImpl(const LedgerFactoryImpl&) = delete;
  LedgerFactoryImpl& operator=(const LedgerFactoryImpl&) = delete;

  ~LedgerFactoryImpl() override;

  void CreateLedger(const base::FilePath& profile,
                    mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
                    mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
                    CreateLedgerCallback callback) override;

 private:
  void LedgerAddedCallback(const base::FilePath& profile,
                           CreateLedgerCallback callback);

  void LedgerRemovedCallback(const base::FilePath& profile);

  mojo::Receiver<mojom::LedgerFactory> receiver_;
  std::map<base::FilePath, base::Thread> ledger_threads_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_LEDGER_FACTORY_IMPL_H_
