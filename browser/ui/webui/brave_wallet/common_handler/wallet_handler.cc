// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/common_handler/wallet_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_wallet {

WalletHandler::WalletHandler(
    mojo::PendingReceiver<mojom::WalletHandler> receiver,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      keyring_service_(KeyringServiceFactory::GetServiceForContext(profile)) {}

WalletHandler::~WalletHandler() = default;

// TODO(apaymyshev): this is the only method in WalletHandler. Should be merged
// into BraveWalletService.
void WalletHandler::GetWalletInfo(GetWalletInfoCallback callback) {
  if (!keyring_service_) {
    std::move(callback).Run(nullptr);
    return;
  }

  auto default_keyring =
      keyring_service_->GetKeyringInfoSync(mojom::kDefaultKeyringId);
  DCHECK_EQ(default_keyring->id, mojom::kDefaultKeyringId);
  std::move(callback).Run(mojom::WalletInfo::New(
      default_keyring->is_keyring_created, default_keyring->is_locked,
      default_keyring->is_backed_up, IsFilecoinEnabled(), IsSolanaEnabled(),
      IsBitcoinEnabled(), IsNftPinningEnabled(), IsPanelV2Enabled()));
}

}  // namespace brave_wallet
