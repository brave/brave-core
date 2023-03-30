/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_FACTORY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_FACTORY_IMPL_H_

#include <memory>

#include "brave/components/services/ledger/public/interfaces/ledger_factory.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ledger {

class LedgerFactoryImpl : public mojom::LedgerFactory {
 public:
  explicit LedgerFactoryImpl(
      mojo::PendingReceiver<mojom::LedgerFactory> receiver);

  ~LedgerFactoryImpl() override;

  LedgerFactoryImpl(const LedgerFactoryImpl&) = delete;
  LedgerFactoryImpl& operator=(const LedgerFactoryImpl&) = delete;

  void CreateLedger(
      mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
      mojo::PendingAssociatedRemote<mojom::RewardsService>
          rewards_service_remote,
      CreateLedgerCallback callback) override;

 private:
  mojo::Receiver<mojom::LedgerFactory> receiver_;
  std::unique_ptr<mojom::Ledger> ledger_;
};

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_FACTORY_IMPL_H_
