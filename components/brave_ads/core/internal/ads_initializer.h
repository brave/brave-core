/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INITIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INITIALIZER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

// Runs the multi-step initialization pipeline (database open, client state
// migration and load, confirmation state migration and load) and signals
// completion via `ResultCallback`.
class AdsInitializer {
 public:
  AdsInitializer();

  AdsInitializer(const AdsInitializer&) = delete;
  AdsInitializer& operator=(const AdsInitializer&) = delete;

  ~AdsInitializer();

  void Initialize(mojom::WalletInfoPtr wallet, ResultCallback callback);

 private:
  void CreateOrOpenDatabase(mojom::WalletInfoPtr wallet,
                            ResultCallback callback);
  void CreateOrOpenDatabaseCallback(mojom::WalletInfoPtr wallet,
                                    ResultCallback callback,
                                    bool success);

  void MigrateClientStateCallback(mojom::WalletInfoPtr wallet,
                                  ResultCallback callback,
                                  bool success);
  void LoadClientStateCallback(mojom::WalletInfoPtr wallet,
                               ResultCallback callback,
                               bool success);
  void MigrateConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                        ResultCallback callback,
                                        bool success);
  void LoadConfirmationStateCallback(mojom::WalletInfoPtr wallet,
                                     ResultCallback callback,
                                     bool success);

  void SuccessfullyInitialized(mojom::WalletInfoPtr wallet,
                               ResultCallback callback);

  base::WeakPtrFactory<AdsInitializer> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INITIALIZER_H_
