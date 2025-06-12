/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;

class CardanoProviderImpl final : public mojom::CardanoProvider {
 public:
  CardanoProviderImpl(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl& operator=(const CardanoProviderImpl&) = delete;
  CardanoProviderImpl(HostContentSettingsMap& host_content_settings_map,
                      BraveWalletService* brave_wallet_service,
                      std::unique_ptr<BraveWalletProviderDelegate> delegate);
  ~CardanoProviderImpl() override;

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
  const raw_ref<HostContentSettingsMap> host_content_settings_map_;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_PROVIDER_IMPL_H_
