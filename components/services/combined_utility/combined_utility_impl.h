/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_COMBINED_UTILITY_COMBINED_UTILITY_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_COMBINED_UTILITY_COMBINED_UTILITY_IMPL_H_

#include "brave/components/services/combined_utility/public/interfaces/combined_utility.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace combined_utility {

class BatAdsLedgerFactoryImpl : public mojom::BatAdsLedgerFactory {
 public:
  explicit BatAdsLedgerFactoryImpl(
      mojo::PendingReceiver<mojom::BatAdsLedgerFactory> receiver);

  ~BatAdsLedgerFactoryImpl() override;

  // mojom::BatAdsLedgerFactory
  void MakeBatLedgerService(
      mojo::PendingReceiver<bat_ledger::mojom::BatLedgerService>
          bat_ledger_receiver) override;

  void MakeBatAdsService(mojo::PendingReceiver<bat_ads::mojom::BatAdsService>
                             bat_ads_receiver) override;

 private:
  mojo::Receiver<mojom::BatAdsLedgerFactory> receiver_;
};

}  // namespace combined_utility
#endif  // BRAVE_COMPONENTS_SERVICES_COMBINED_UTILITY_COMBINED_UTILITY_IMPL_H_
