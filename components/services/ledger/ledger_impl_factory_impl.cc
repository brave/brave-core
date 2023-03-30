/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ledger/ledger_impl_factory_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {

LedgerImplFactoryImpl::LedgerImplFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerImplFactory> receiver)
    : receiver_(this, std::move(receiver)) {}

LedgerImplFactoryImpl::~LedgerImplFactoryImpl() = default;

void LedgerImplFactoryImpl::CreateLedger(
    mojo::PendingAssociatedReceiver<rewards::mojom::RewardsUtilityService>
        rewards_utility_service,
    mojo::PendingAssociatedRemote<rewards::mojom::RewardsService>
        rewards_service,
    CreateLedgerCallback callback) {
  utility_service_ = std::make_unique<LedgerImpl>(
      std::move(rewards_utility_service), std::move(rewards_service));
  std::move(callback).Run();
}

}  // namespace ledger
