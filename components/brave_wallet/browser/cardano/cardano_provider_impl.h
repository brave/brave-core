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

class BraveWalletProviderDelegate;

class CardanoProviderImpl final : public mojom::CardanoProvider,
                                  public KeyringServiceObserverBase {
 public:
  CardanoProviderImpl(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl& operator=(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl(BraveWalletService& brave_wallet_service_,
                      std::unique_ptr<BraveWalletProviderDelegate> delegate);
  ~CardanoProviderImpl() override;

  // mojom::CardanoProvider
  void Enable(EnableCallback callback) override;
  void IsEnabled(IsEnabledCallback callback) override;

  void GetNetworkId(GetNetworkIdCallback callback) override;
  void GetUsedAddresses(GetUsedAddressesCallback callback) override;
  void GetUnusedAddresses(GetUnusedAddressesCallback callback) override;
  void GetChangeAddress(GetChangeAddressCallback callback) override;
  void GetRewardAddresses(GetRewardAddressesCallback callback) override;
  void GetBalance(GetBalanceCallback callback) override;
  void GetUtxos(const std::optional<std::string>& amount,
                mojom::CardanoProviderPaginationPtr paginate,
                GetUtxosCallback callback) override;
  void SignTx(const std::string& tx_cbor,
              bool partial_sign,
              SignTxCallback callback) override;
  void SubmitTx(const std::string& signed_tx_cbor,
                SubmitTxCallback callback) override;
  void SignData(const std::string& address,
                const std::string& payload_hex,
                SignDataCallback callback) override;
  void GetCollateral(const std::string& amount,
                     GetCollateralCallback callback) override;

 private:
  friend class CardanoProviderImplUnitTest;

  mojom::AccountIdPtr GetAllowedSelectedAccount();

  BraveWalletProviderDelegate* delegate();

  // KeyringServiceObserverBase:
  void Locked() override;
  void Unlocked() override;
  void SelectedDappAccountChanged(mojom::CoinType coin,
                                  mojom::AccountInfoPtr account) override;

  void RequestCardanoPermissions(EnableCallback callback,
                                 const url::Origin& origin);

  void OnRequestCardanoPermissions(
      EnableCallback callback,
      const url::Origin& origin,
      mojom::RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts);

  raw_ref<BraveWalletService> brave_wallet_service_;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;

  std::optional<EnableCallback> pending_request_cardano_permissions_callback_;

  url::Origin pending_request_cardano_permissions_origin_;
  bool wallet_onboarding_shown_ = false;

  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};

  base::WeakPtrFactory<CardanoProviderImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_
