/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_BRAVE_WALLET_UTILS_SERVICE_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_BRAVE_WALLET_UTILS_SERVICE_H_

#include "brave/components/services/brave_wallet/public/mojom/brave_wallet_utils_service.mojom.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

/**
 * Launches and communicates mojom::BraveWalletUtilsService in separate process.
 */
class BraveWalletUtilsService {
 public:
  BraveWalletUtilsService();
  ~BraveWalletUtilsService();
  BraveWalletUtilsService(const BraveWalletUtilsService&) = delete;
  BraveWalletUtilsService& operator=(const BraveWalletUtilsService&) = delete;

  // Creates decoder in brave wallet utils process and provides handles.
  void CreateZCashDecoder(
      mojo::PendingAssociatedReceiver<zcash::mojom::ZCashDecoder> receiver);

  static BraveWalletUtilsService* GetInstance();

 private:
  void MaybeLaunchService();

  mojo::Remote<mojom::BraveWalletUtilsService> brave_wallet_utils_service_;

  base::WeakPtrFactory<BraveWalletUtilsService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_BRAVE_WALLET_UTILS_SERVICE_H_
