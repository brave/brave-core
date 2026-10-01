/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_API_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_API_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

// The `Injected` object a dapp holds after `enable()` resolved. Bound to the
// account the user granted, and only usable while that permission stands.
class PolkadotApiImpl final : public mojom::PolkadotApi {
 public:
  PolkadotApiImpl(BraveWalletService& brave_wallet_service,
                  std::unique_ptr<BraveWalletProviderDelegate> delegate,
                  mojom::AccountIdPtr granted_account,
                  const url::Origin& origin);
  ~PolkadotApiImpl() override;
  PolkadotApiImpl(const PolkadotApiImpl&) = delete;
  PolkadotApiImpl& operator=(const PolkadotApiImpl&) = delete;

  // mojom::PolkadotApi
  void GetAccounts(bool any_type, GetAccountsCallback callback) override;
  void SignPayload(const std::string& payload_json,
                   SignPayloadCallback callback) override;

 private:
  // Whether the granted account is still permitted for this origin. The
  // permission can be revoked, or expire, while the dapp holds this object.
  bool IsGrantedAccountAllowed();

  void OnResolveChainIdForSignPayload(
      std::string payload_json,
      SignPayloadCallback callback,
      std::optional<std::string> chain_id);

  void OnGetMetadataForSignPayload(
      std::string payload_json,
      std::string chain_id,
      SignPayloadCallback callback,
      base::expected<std::vector<uint8_t>, std::string> metadata_bytes);

  void OnSignPayloadRequestProcessed(SignPayloadCallback callback,
                                     bool approved,
                                     const std::optional<std::string>& error);

  const raw_ref<BraveWalletService> brave_wallet_service_;
  // Owns its own delegate, which knows the frame the dapp is in.
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  const mojom::AccountIdPtr granted_account_;
  const url::Origin origin_;

  base::WeakPtrFactory<PolkadotApiImpl> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_API_IMPL_H_
