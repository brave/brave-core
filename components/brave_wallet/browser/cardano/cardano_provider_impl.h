/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class CardanoProviderImpl final : public mojom::CardanoProvider,
                                  public KeyringServiceObserverBase {
 public:
  using BraveWalletProviderDelegateFactory =
      base::RepeatingCallback<std::unique_ptr<BraveWalletProviderDelegate>()>;

  CardanoProviderImpl(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl& operator=(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl(BraveWalletService& brave_wallet_service,
                      BraveWalletProviderDelegateFactory delegate_factory);
  ~CardanoProviderImpl() override;

  // mojom::CardanoProvider
  void Enable(mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
              EnableCallback callback) override;
  void IsEnabled(IsEnabledCallback callback) override;


 private:
  friend class CardanoProviderImplUnitTest;

  mojom::AccountIdPtr GetAllowedSelectedAccount();

  BraveWalletProviderDelegate* delegate();

  // KeyringServiceObserverBase:
  void Locked() override;
  void Unlocked() override;
  void SelectedDappAccountChanged(mojom::CoinType coin,
                                  mojom::AccountInfoPtr account) override;

  void RequestCardanoPermissions(
      mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
      EnableCallback callback,
      const url::Origin& origin);

  void OnRequestCardanoPermissions(
      mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
      EnableCallback callback,
      const url::Origin& origin,
      mojom::RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts);

  raw_ref<BraveWalletService> brave_wallet_service_;
  BraveWalletProviderDelegateFactory delegate_factory_;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;

  std::optional<EnableCallback> pending_request_cardano_permissions_callback_;
  mojo::PendingReceiver<mojom::CardanoApi> pending_cardano_api_;
  url::Origin pending_request_cardano_permissions_origin_;
  bool wallet_onboarding_shown_ = false;

  mojo::Receiver<mojom::KeyringServiceObserver> keyring_observer_receiver_{
      this};

  base::WeakPtrFactory<CardanoProviderImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_
