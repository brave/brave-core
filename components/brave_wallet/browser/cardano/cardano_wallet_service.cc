/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <stdint.h>

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

CardanoWalletService::CardanoWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service) {}

CardanoWalletService::~CardanoWalletService() = default;

void CardanoWalletService::Bind(
    mojo::PendingReceiver<mojom::CardanoWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void CardanoWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void CardanoWalletService::GetBalance(mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  CHECK(IsCardanoAccount(account_id));
  std::move(callback).Run(mojom::CardanoBalance::New(0), std::nullopt);
}

}  // namespace brave_wallet
