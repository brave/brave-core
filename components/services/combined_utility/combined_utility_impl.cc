/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/combined_utility/combined_utility_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"
#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"

namespace combined_utility {

BatAdsLedgerFactoryImpl::BatAdsLedgerFactoryImpl(
    mojo::PendingReceiver<mojom::BatAdsLedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {}

BatAdsLedgerFactoryImpl::~BatAdsLedgerFactoryImpl() = default;

void BatAdsLedgerFactoryImpl::MakeBatLedgerService(
    mojo::PendingReceiver<bat_ledger::mojom::BatLedgerService>
        bat_ledger_receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<bat_ledger::BatLedgerServiceImpl>(),
      std::move(bat_ledger_receiver));
}

void BatAdsLedgerFactoryImpl::MakeBatAdsService(
    mojo::PendingReceiver<bat_ads::mojom::BatAdsService> bat_ads_receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<bat_ads::BatAdsServiceImpl>(),
                              std::move(bat_ads_receiver));
}

}  // namespace combined_utility
