/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <string>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace brave_wallet {

class BraveWalletImporterDelegate;

class BraveWalletService : public KeyedService,
                           public mojom::BraveWalletService {
 public:
  explicit BraveWalletService(
      std::unique_ptr<BraveWalletImporterDelegate> delegate,
      PrefService* prefs);
  ~BraveWalletService() override;

  BraveWalletService(const BraveWalletService&) = delete;
  BraveWalletService& operator=(const BraveWalletService&) = delete;

  mojo::PendingRemote<mojom::BraveWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletService> receiver);

  // mojom::BraveWalletService:
  void GetUserAssets(const std::string& chain_id,
                     GetUserAssetsCallback callback) override;
  void AddUserAsset(mojom::ERCTokenPtr token,
                    const std::string& chain_id,
                    AddUserAssetCallback callback) override;
  void RemoveUserAsset(const std::string& contract_address,
                       const std::string& chain_id,
                       RemoveUserAssetCallback callback) override;
  void SetUserAssetVisible(const std::string& contract_address,
                           const std::string& chain_id,
                           bool visible,
                           SetUserAssetVisibleCallback callback) override;
  void IsCryptoWalletsInstalled(
      IsCryptoWalletsInstalledCallback callback) override;
  void IsMetaMaskInstalled(IsMetaMaskInstalledCallback callback) override;
  void ImportFromCryptoWallets(
      const std::string& password,
      const std::string& new_password,
      ImportFromCryptoWalletsCallback callback) override;
  void ImportFromMetaMask(const std::string& password,
                          const std::string& new_password,
                          ImportFromMetaMaskCallback callback) override;
  void GetDefaultWallet(GetDefaultWalletCallback callback) override;
  void SetDefaultWallet(mojom::DefaultWallet default_wallet) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetChecksumAddress);

  absl::optional<std::string> GetChecksumAddress(
      const std::string& contract_address,
      const std::string& chain_id);

  std::unique_ptr<BraveWalletImporterDelegate> delegate_;
  PrefService* prefs_;
  mojo::ReceiverSet<mojom::BraveWalletService> receivers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
