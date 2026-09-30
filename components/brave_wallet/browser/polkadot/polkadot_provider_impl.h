/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_PROVIDER_IMPL_H_

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
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "url/origin.h"

namespace brave_wallet {

// Backs `window.injectedWeb3['brave-wallet']`. `enable()` is the permission
// gate: it resolves to a `PolkadotApi` once the origin holds the Polkadot dapp
// permission for an account, having prompted the user to pick one if needed.
// https://github.com/polkadot-js/extension#injection-information
class PolkadotProviderImpl final : public mojom::PolkadotProvider,
                                   public KeyringServiceObserverBase {
 public:
  using BraveWalletProviderDelegateFactory =
      base::RepeatingCallback<std::unique_ptr<BraveWalletProviderDelegate>()>;

  PolkadotProviderImpl(BraveWalletService& brave_wallet_service,
                       BraveWalletProviderDelegateFactory delegate_factory,
                       const url::Origin& origin);
  ~PolkadotProviderImpl() override;
  PolkadotProviderImpl(const PolkadotProviderImpl&) = delete;
  PolkadotProviderImpl& operator=(const PolkadotProviderImpl&) = delete;

  // mojom::PolkadotProvider
  void Enable(EnableCallback callback) override;

 private:
  enum class PermissionCheckResult {
    kTabInactive,
    kDeniedGlobally,
    kWalletNotCreated,
    kNoAccounts,
    kWalletLocked,
    kGetAllowedAccountsFailed,
    kHasAllowedAccounts,
    kNeedsPermissionRequest
  };

  friend class PolkadotProviderImplUnitTest;

  PermissionCheckResult EvaluatePermissionsState(
      std::vector<std::string>& allowed_accounts);

  void RequestPolkadotPermissions(EnableCallback callback,
                                  const url::Origin& origin);

  void OnRequestPolkadotPermissions(
      EnableCallback callback,
      mojom::RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts);

  // KeyringServiceObserverBase:
  void Unlocked() override;

  raw_ref<BraveWalletService> brave_wallet_service_;
  BraveWalletProviderDelegateFactory delegate_factory_;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  const url::Origin origin_;

  // Held while waiting for the user to unlock the wallet.
  EnableCallback pending_request_permissions_callback_;
  url::Origin pending_request_permissions_origin_;
  bool account_creation_shown_ = false;

  mojo::Receiver<mojom::KeyringServiceObserver> keyring_observer_receiver_{
      this};

  mojo::UniqueReceiverSet<mojom::PolkadotApi> polkadot_api_receivers_;

  base::WeakPtrFactory<PolkadotProviderImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_PROVIDER_IMPL_H_
