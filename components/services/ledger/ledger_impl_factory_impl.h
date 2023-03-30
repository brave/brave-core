/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_IMPL_FACTORY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_IMPL_FACTORY_IMPL_H_

#include <memory>

#include "brave/components/services/ledger/public/interfaces/ledger_impl_factory.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace ledger {

class LedgerImplFactoryImpl : public mojom::LedgerImplFactory {
 public:
  explicit LedgerImplFactoryImpl(
      mojo::PendingReceiver<mojom::LedgerImplFactory> receiver);

  ~LedgerImplFactoryImpl() override;

  LedgerImplFactoryImpl(const LedgerImplFactoryImpl&) = delete;
  LedgerImplFactoryImpl& operator=(const LedgerImplFactoryImpl&) = delete;

  void CreateLedger(
      mojo::PendingAssociatedReceiver<rewards::mojom::RewardsUtilityService>
          rewards_utility_service,
      mojo::PendingAssociatedRemote<rewards::mojom::RewardsService>
          rewards_service,
      CreateLedgerCallback callback) override;

 private:
  mojo::Receiver<mojom::LedgerImplFactory> receiver_;
  std::unique_ptr<rewards::mojom::RewardsUtilityService> utility_service_;
};

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_SERVICES_LEDGER_LEDGER_IMPL_FACTORY_IMPL_H_
